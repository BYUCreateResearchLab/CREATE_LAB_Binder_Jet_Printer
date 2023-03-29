#include "mfjdrv.h"

#include <cmath>

#define MFJDRV_FREQBASE 		625000.0

#define MFJDRV_ACK				0x06
#define MFJDRV_NAK				0x15

namespace
{
constexpr int max(int a, int b);
constexpr unsigned char high_byte(short val);
constexpr unsigned char low_byte(short val);
constexpr signed char signed_high_byte(short val);
constexpr signed char signed_low_byte(short val);
}

namespace JetDrive
{

CommandBuilder::CommandBuilder() {}

const QByteArray CommandBuilder::build(
        CMD command,
        Settings &jetParams)
{
    long LongFreq;
    int IntTime;
    Waveform &waveform = jetParams.waveform;

    QByteArray jetCmd;
    jetCmd.resize(3);
    jetCmd[0] = 'S';
    jetCmd[2] = low_byte((uchar)command);

    switch (command)
    {

    case CMD::NOCOMMAND:
        jetCmd.resize(0);
        break;

    case CMD::RESET:
        break;

    case CMD::POLLSTATUS:
        break;

    case CMD::DROPS:
        if (gDreamController || jetParams.fDrops > 255)
        {
            jetCmd.resize(5);
            jetCmd[3] = high_byte(jetParams.fDrops);
            jetCmd[4] = low_byte(jetParams.fDrops);
        }
        else
        {
            jetCmd.resize(4);
            jetCmd[3] = low_byte(jetParams.fDrops);
        }
        break;

    case CMD::CONTMODE:
        jetCmd.resize(4);
        jetCmd[3] = (uchar)(jetParams.fMode ? 0x01 : 0x00);
        break;

    case CMD::FREQUENCY:
        if (jetParams.fFrequency < 1L)
            jetParams.fFrequency = 1L;
        LongFreq = (long)(MFJDRV_FREQBASE / (double)jetParams.fFrequency - 0.5);
        if (LongFreq < 1L)
            LongFreq = 1L;
        if ((gFirmVersion < 40) && (LongFreq > 4095L))
            LongFreq = 4095L;
        if (LongFreq <= 4095L)
        {
            jetCmd.resize(5);
            jetCmd[3] = high_byte(LongFreq);
            jetCmd[4] = low_byte(LongFreq);
        }
        else
        {
            jetCmd.resize(4);
            jetCmd[2] = (uchar)(CMD::LOWFREQ);
            jetCmd[3] = low_byte(jetParams.fFrequency);
        }
        break;

    case CMD::PULSE:
        jetCmd.resize(22);
        IntTime = (int)(waveform.fTDwell * 10.0 + 0.001);
        jetCmd[5] = high_byte(IntTime);
        jetCmd[6] = low_byte(IntTime);
        IntTime = (int)(waveform.fTEcho * 10.0 + 0.001);
        jetCmd[8] = high_byte(IntTime);
        jetCmd[9] = low_byte(IntTime);

        if (gDreamController ||
                abs(waveform.fUIdle) > 100 ||
                abs(waveform.fUDwell) > 100 ||
                abs(waveform.fUEcho) > 100 ||
                waveform.fTRise > 0.11 ||
                waveform.fTFall > 0.11 ||
                waveform.fTFinal > 0.11)
        {
            jetCmd[ 3] = 0x00;
            jetCmd[ 4] = 0x00;
            jetCmd[ 7] = 0x00;
            jetCmd[10] = signed_high_byte(waveform.fUIdle);
            jetCmd[11] = signed_low_byte(waveform.fUIdle);
            jetCmd[12] = signed_high_byte(waveform.fUDwell);
            jetCmd[13] = signed_low_byte(waveform.fUDwell);
            jetCmd[14] = signed_high_byte(waveform.fUEcho);
            jetCmd[15] = signed_low_byte(waveform.fUEcho);
            int minTime = (int)(fabs(((double)
                                      (waveform.fUDwell-waveform.fUIdle) /
                                      gRateLimit)) * 10.0) + 10;
            IntTime = max((int)(waveform.fTRise * 10.0 + 0.001), minTime);
            jetCmd[16] = high_byte(IntTime);
            jetCmd[17] = low_byte(IntTime);
            minTime = (int)(fabs(((double)
                                  (waveform.fUEcho-waveform.fUDwell) /
                                  gRateLimit)) * 10.0) + 10;
            IntTime = max((int)(waveform.fTFall * 10.0 + 0.001), minTime);
            jetCmd[18] = high_byte(IntTime);
            jetCmd[19] = low_byte(IntTime);
            minTime = (int)(fabs(((double)
                                  (waveform.fUIdle-waveform.fUEcho) /
                                  gRateLimit)) * 10.0) + 10;
            IntTime = max((int)(waveform.fTFinal * 10.0 + 0.001), minTime);
            jetCmd[20] = high_byte(IntTime);
            jetCmd[21] = low_byte(IntTime);
        }
        else
        {
            jetCmd[3] = signed_low_byte(waveform.fUIdle);
            jetCmd[4] = signed_low_byte(waveform.fUDwell);
            if ( (waveform.fUEcho - waveform.fUIdle)
                    * (waveform.fUDwell - waveform.fUIdle) < 0 )
            {
                jetCmd.resize(10);
                jetCmd[7] = signed_low_byte(waveform.fUEcho);
            }
            else
            {
                jetCmd.resize(7);
            }
        }
        break;

    case CMD::STROBEDIV:
        jetCmd.resize(4);
        jetCmd[3] = low_byte(jetParams.fStrobeDiv);
        break;

    case CMD::SOURCE:
        jetCmd.resize(4);
        if (gMultiChannel)
            jetCmd[3] = (uchar)(jetParams.fChannelOn ? 0x01 : 0x00);
        else
            jetCmd[3] = (uchar)(jetParams.fSource ? 0x01 : 0x00);
        break;

    case CMD::SOFTTRIGGER:
        break;

    case CMD::MULTITRIGGER:
        break;

    case CMD::EXTERNENABLE:
        jetCmd.resize(4);
        if (!gMultiChannel) jetCmd[2] = (uchar)(CMD::SOURCE);
        jetCmd[3] = (uchar)(gExternEnable ? 0x01 : 0x00);
        break;

    case CMD::STROBEENABLE:
        jetCmd.resize(4);
        jetCmd[3] = (uchar)(jetParams.fStrobeEnable ? 0x01 : 0x00);
        break;

    case CMD::LOWFREQ:
        if (jetParams.fFrequency >= 256L || gFirmVersion < 40)
        {
            jetCmd.resize(-1); // WHY -1 HERE??
        }
        else
        {
            jetCmd.resize(4);
            jetCmd[3] = low_byte(jetParams.fFrequency);
        }
        break;

    case CMD::FULLFREQ:
        if (jetParams.fFrequency >= 65536L) jetCmd.resize(-1); // -1 HERE TOO
        else
        {
            jetCmd.resize(5);
            jetCmd[3] = high_byte(jetParams.fFrequency);
            jetCmd[4] = low_byte(jetParams.fFrequency);
        }
        break;

    case CMD::STROBEDELAY:
        jetCmd.resize(6);
        jetCmd[3] = 0x01; // Keep potentiometer on at all times.
        jetCmd[4] = high_byte(jetParams.fStrobeDelay);
        jetCmd[5] = low_byte(jetParams.fStrobeDelay);
        break;

    case CMD::DUMPINPUT:
        break;

    case CMD::DEBUG:
        jetCmd.resize(5);
        jetCmd[3] = low_byte(jetParams.fDebugSwitch);
        jetCmd[4] = low_byte(jetParams.fDebugValue);
        break;

    case CMD::POKE:
        jetCmd.append("\x40" "\x07" "\x0D");
        break;

    case CMD::GETVERSION:
        break;

    default:
        jetCmd.resize(-1);
    }

    if (jetCmd.size() > 0)
    {
        if (jetCmd.size() > 1)
        {
            // number of bytes (not including header 'S')
            jetCmd[1] = (uchar)(jetCmd.size() - 1);
            gCheckLong = 0L;
            for (int i=1; i<jetCmd.size(); i++)
                gCheckLong += (long)jetCmd[i];
            // add check sum
            jetCmd.append(low_byte(gCheckLong));
        }
        else jetCmd.resize(0);
    }

    return jetCmd;
}

}

namespace
{

constexpr int max(int a, int b)
{ return ((a) > (b)) ? (a) : (b); }

constexpr uchar high_byte(short val)
{ return (uchar)((val >> 8) & 0x00FF); }

constexpr uchar low_byte(short val)
{ return (uchar)(val & 0x00FF); }

constexpr signed char signed_high_byte(short val)
{ return (signed char)((val >> 8) & 0x00FF); }

constexpr signed char signed_low_byte(short val)
{ return (signed char)(val & 0x00FF); }

}

