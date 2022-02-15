#include "jetdrive.h"

#include <windows.h>
#include <ctype.h>
#include <sys/timeb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <QDebug>

#define _CRT_SECURE_NO_WARNINGS

#define MFJDRV_NOCOMMAND		0x00
#define MFJDRV_RESET			0x01
#define MFJDRV_POLLSTATUS		0x02
#define MFJDRV_DROPS			0x03
#define MFJDRV_CONTMODE			0x04
#define MFJDRV_FREQUENCY		0x05
#define MFJDRV_PULSE			0x06
#define MFJDRV_STROBEDIV		0x07
#define MFJDRV_SOURCE			0x08
#define MFJDRV_SOFTTRIGGER		0x09
#define MFJDRV_STROBEENABLE     0x10
#define MFJDRV_LOWFREQ			0x11
#define MFJDRV_FULLFREQ			0x12
#define MFJDRV_STROBEDELAY		0x13
#define MFJDRV_DUMPINPUT		0x60
#define MFJDRV_DEBUG			0x61
#define MFJDRV_POKE				0xEF
#define MFJDRV_GETVERSION		0xF0

#define MFJDRV_FREQBASE 		625000.0
#define MFJDRV_ACK				0x06
#define MFJDRV_NAK				0x15

#define MFJDRV_DUMPLENGTH		29

#define MFJDRV_MULTITRIGGER     0x0D
#define MFJDRV_EXTERNENABLE     0x0C
#define MFJDRV_EDITCHANNEL      0x0E
// I don't think these 3 are the right commands, but I'm not sure it matters for our JetDrive...

#define gDreamController true
#define gMultiChannel false
#define gBreadboardOne false
//DOES THIS WORK??

short MicroJet::fgNJets = 0;

MicroJet::MicroJet() :
    fTRise(3.0),
    fTDwell(20.0),
    fTFall(4.0),
    fTEcho(40.0),
    fTFinal(3.0),
    fTDelay(0.0),
    fFrequency(1000L),// fFrequency is for setting the jetting frequency (in Hz?)
    fUIdle(0),
    fUDwell(25),      // Dwell Voltage
    fUEcho(-25),      // Echo Voltage
    fUGain(225),
    fMode(0),         // fmode is to set continuous jetting mode (1 to enable continuous jetting, 0 to disable)
    fSource(1),       // fSource is for setting the trigger source (1 for external TTL trigger, 0 for internal trigger)
    fDrops(1),
    fStrobeDelay(0),  // sets strobe delay in microseconds
    fStrobeDiv(1),    // divider for the stobe (1 means the strobe flashes every pulse, 2 means the strobe flashes every other jetting pulse)
    fStrobeEnable(1),
    fDebugSwitch(0),
    fDebugValue(0),
    fChannelGroup(0),
    fContIsStarted(0),
    fChannelOn(0)
{
    fgNJets++;
}


MicroJet::~MicroJet()
{
    fgNJets--;
}

JetDrive::JetDrive() :
    hCom(INVALID_HANDLE_VALUE),
    noCom(INVALID_HANDLE_VALUE)
{
    mJetSettings = new MicroJet();
}

const char JetDrive::fmtCommError[] = "%s failed with error %ld.\n";
const int JetDrive::NOCOM = -1;

JetDrive::~JetDrive()
{
    delete mJetSettings;
}

void JetDrive::wait_seconds(float seconds)
{
    Sleep(seconds * 1000);
}

void JetDrive::test_log(const char* outputstring, bool status)
{
    qDebug() << outputstring;
}

// Function Declarations
int JetDrive::build_command(int command, unsigned char *jetCommand)
{
// Build a Stortz command from current parameters.
//
// (C) 1997-2000 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.

    long LongFreq;
    int commandLength, i, IntTime;
    //char CommandText[255];

    commandLength = 3;
    jetCommand[0] = 'S';
    jetCommand[2] = (unsigned char)(command & 0xFF);

    switch (command)
    {

    case MFJDRV_NOCOMMAND :
        jetCommand[0] = '\0';
        commandLength = 0;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\n(No command)");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_RESET :
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nReset");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_POLLSTATUS :
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nPoll status");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_DROPS :
        if (gDreamController || mJetSettings->fDrops > 255)
        {
            jetCommand[3] = (unsigned char)((mJetSettings->fDrops >> 8) & 0x00FF);
            jetCommand[4] = (unsigned char)(mJetSettings->fDrops & 0x00FF);
            commandLength = 5;
        }
        else
        {
            jetCommand[3] = (unsigned char)(mJetSettings->fDrops & 0x00FF);
            commandLength = 4;
        }
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet drops/trigger to %d.",
                mJetSettings->fDrops);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_CONTMODE : // continuous trigger mode (fMode = 1 for continuous, fMode = 0 for single)
        jetCommand[3] = (unsigned char)(mJetSettings->fMode ? 0x01 : 0x00);
        commandLength = 4;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet trigger mode to %s.",
                mJetSettings->fMode ? "continuous" : "single");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_FREQUENCY :
        if (mJetSettings->fFrequency < 1L) mJetSettings->fFrequency = 1L;
        LongFreq = (long)(MFJDRV_FREQBASE / (double)mJetSettings->fFrequency -
                          0.5);
        if (LongFreq < 1L) LongFreq = 1L;
        if ((gFirmVersion < 40) && (LongFreq > 4095L))
        {
            LongFreq = 4095L;
        }
        if (LongFreq <= 4095L)
        {
            jetCommand[3] = (unsigned char)((LongFreq >> 8) & 0x000000FF); // high byte
            jetCommand[4] = (unsigned char)(LongFreq & 0x000000FF);        // low byte
            commandLength = 5;
        }
        else
        {
            jetCommand[2] = MFJDRV_LOWFREQ;
            jetCommand[3] = (unsigned char)(mJetSettings->fFrequency & 0x000000FF);
            commandLength = 4;
        }
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet frequency to %ld Hz (divider = %ld).",
                mJetSettings->fFrequency, LongFreq);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_PULSE :
        IntTime = (int)(mJetSettings->fTDwell * 10.0 + 0.001);
        jetCommand[5] = (unsigned char)((IntTime >> 8) & 0x00FF);
        jetCommand[6] = (unsigned char)(IntTime & 0x00FF);
        IntTime = (int)(mJetSettings->fTEcho * 10.0 + 0.001);
        jetCommand[8] = (unsigned char)((IntTime >> 8) & 0x00FF);
        jetCommand[9] = (unsigned char)(IntTime & 0x00FF);
        if (gDreamController ||
                abs(mJetSettings->fUIdle) > 100 ||
                abs(mJetSettings->fUDwell) > 100 ||
                abs(mJetSettings->fUEcho) > 100 ||
                mJetSettings->fTRise > 0.11 ||
                mJetSettings->fTFall > 0.11 ||
                mJetSettings->fTFinal > 0.11)
        {
            jetCommand[ 3] = 0x00;
            jetCommand[ 4] = 0x00;
            jetCommand[ 7] = 0x00;
            jetCommand[10] = (signed char)((mJetSettings->fUIdle >> 8) & 0x00FF);
            jetCommand[11] = (signed char)(mJetSettings->fUIdle & 0x00FF);
            jetCommand[12] = (signed char)((mJetSettings->fUDwell >> 8) &
                                           0x00FF);
            jetCommand[13] = (signed char)(mJetSettings->fUDwell & 0x00FF);
            jetCommand[14] = (signed char)((mJetSettings->fUEcho >> 8) & 0x00FF);
            jetCommand[15] = (signed char)(mJetSettings->fUEcho & 0x00FF);
            int minTime = (int)(fabs(((double)
                                      (mJetSettings->fUDwell-mJetSettings->fUIdle) /
                                      gRateLimit)) * 10.0) + 10;
            IntTime = max((int)(mJetSettings->fTRise * 10.0 + 0.001), minTime);
            jetCommand[16] = (unsigned char)((IntTime >> 8) & 0x00FF);
            jetCommand[17] = (unsigned char)(IntTime & 0x00FF);
            minTime = (int)(fabs(((double)
                                  (mJetSettings->fUEcho-mJetSettings->fUDwell) /
                                  gRateLimit)) * 10.0) + 10;
            IntTime = max((int)(mJetSettings->fTFall * 10.0 + 0.001), minTime);
            jetCommand[18] = (unsigned char)((IntTime >> 8) & 0x00FF);
            jetCommand[19] = (unsigned char)(IntTime & 0x00FF);
            minTime = (int)(fabs(((double)
                                  (mJetSettings->fUIdle-mJetSettings->fUEcho) /
                                  gRateLimit)) * 10.0) + 10;
            IntTime = max((int)(mJetSettings->fTFinal * 10.0 + 0.001), minTime);
            jetCommand[20] = (unsigned char)((IntTime >> 8) & 0x00FF);
            jetCommand[21] = (unsigned char)(IntTime & 0x00FF);
            commandLength = 22;
        }
        else
        {
            jetCommand[3] = (signed char)(mJetSettings->fUIdle & 0x00FF);
            jetCommand[4] = (signed char)(mJetSettings->fUDwell & 0x00FF);
            if ((mJetSettings->fUEcho - mJetSettings->fUIdle) * (mJetSettings->fUDwell - mJetSettings->fUIdle) < 0)
            {
                jetCommand[7] = (signed char)(mJetSettings->fUEcho & 0x00FF);
                commandLength = 10;
            }
            else
            {
                commandLength = 7;
            }
        }
#ifdef MFDRV_DEBUG
        sprintf(CommandText,
                "\nSet waveform to U=%d/%d/%d V, t=%.1lf/%.1lf/%.1lf/%.1lf/%.1lf %s.",
                mJetSettings->fUIdle, mJetSettings->fUDwell, mJetSettings->fUEcho,
                mJetSettings->fTRise, mJetSettings->fTDwell, mJetSettings->fTFall,
                mJetSettings->fTEcho, mJetSettings->fTFinal, gMicroS);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_STROBEDIV :
        jetCommand[3] = (unsigned char)(mJetSettings->fStrobeDiv & 0x00FF);
        commandLength = 4;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet strobe divider to %d.",
                mJetSettings->fStrobeDiv);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_SOURCE :
        if (gMultiChannel)
        {
            jetCommand[3] = (unsigned char)(mJetSettings->fChannelOn ? 0x01 : 0x00);
#ifdef MFDRV_DEBUG
            sprintf(CommandText, "\n%sclude channel %d %s multi-trigger set.",
                    mJetSettings->fChannelOn ? "In" : "Ex", gCJ+1,
                    mJetSettings->fChannelOn ? "into" : "from");
#endif
        }
        else
        {
            jetCommand[3] = (unsigned char)(mJetSettings->fSource ? 0x01 : 0x00);
#ifdef MFDRV_DEBUG
            sprintf(CommandText, "\nSet trigger source to %s.",
                    mJetSettings->fSource ? "External" : "PC Trig.");
#endif  //  ifdef MFDRV_DEBUG
        }
        commandLength = 4;
        break;

    case MFJDRV_SOFTTRIGGER :
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nTrigger output.");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_MULTITRIGGER :
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nMulti-trigger output.");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_EXTERNENABLE :
        if (!gMultiChannel) jetCommand[2] = MFJDRV_SOURCE;
        jetCommand[3] = (unsigned char)(gExternEnable ? 0x01 : 0x00);
        commandLength = 4;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\n%sable external trigger.",
                gExternEnable ? "En" : "Dis");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_STROBEENABLE :
        jetCommand[3] = (unsigned char)(mJetSettings->fStrobeEnable ? 0x01 : 0x00);
        commandLength = 4;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\n%sable strobe.",
                mJetSettings->fStrobeEnable ? "En" : "Dis");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_LOWFREQ :
        if (mJetSettings->fFrequency >= 256L || gFirmVersion < 40)
        {
            commandLength = -1;
        }
        else
        {
            jetCommand[3] = (unsigned char)(mJetSettings->fFrequency & 0x000000FF);
            commandLength = 4;
        }
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet low frequency to %ld Hz.",
                mJetSettings->fFrequency);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_FULLFREQ :	// new with DREAM design
        if (mJetSettings->fFrequency >= 65536L)
        {
            commandLength = -1;
        }
        else
        {
            jetCommand[3] = (unsigned char)((mJetSettings->fFrequency >> 8) & 0x000000FF);
            jetCommand[4] = (unsigned char)(mJetSettings->fFrequency & 0x000000FF);
            commandLength = 5;
        }
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet frequency to %ld Hz (vs.5).",
                mJetSettings->fFrequency);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_STROBEDELAY :	// new with DREAM design
        jetCommand[3] = 0x01;	// Keep potentiometer on at all times.
        jetCommand[4] = (unsigned char)((mJetSettings->fStrobeDelay >> 8) & 0x00FF);
        jetCommand[5] = (unsigned char)(mJetSettings->fStrobeDelay & 0x00FF);
        commandLength = 6;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nSet strobe delay to %d %s.",
                mJetSettings->fStrobeDelay, gMicroS);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_DUMPINPUT :	// new with DREAM design
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nRequest dump of settings.");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_DEBUG :	// new with DREAM design
        jetCommand[3] = (unsigned char)(mJetSettings->fDebugSwitch & 0xFF);
        jetCommand[4] = (unsigned char)(mJetSettings->fDebugValue & 0xFF);
        commandLength = 5;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nDebug for switch %02Xh value %02Xh.",
                JetCommand[3], JetCommand[4]);
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_POKE :
        jetCommand[3] = 0x40;
        jetCommand[4] = 0x07;
        jetCommand[5] = 0x0D;
        commandLength = 6;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nPoke");
#endif  //  ifdef MFDRV_DEBUG
        break;

    case MFJDRV_GETVERSION :
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nGet version");
#endif  //  ifdef MFDRV_DEBUG
        break;

    default:
        commandLength = -1;
#ifdef MFDRV_DEBUG
        sprintf(CommandText, "\nCommand not recognized: %04Xh", Command);
#endif  //  ifdef MFDRV_DEBUG
        break;
    }

    if (commandLength > 0)
    {
#ifdef MFDRV_DEBUG
        TestLog(CommandText, true);
#endif  //  ifdef MFDRV_DEBUG
        if (commandLength > 1)
        {
            jetCommand[1] = (unsigned char)(commandLength - 1);
            gCheckLong = 0L;
            for (i=1; i<commandLength; i++)
                gCheckLong += (long)jetCommand[i];
            jetCommand[commandLength++] = (unsigned char)(gCheckLong & 0x000000FF);
        }
        else
        {
            commandLength = 0;
        }
    }
    return commandLength;
}

int JetDrive::get_from_jet_drive(int port, int command, unsigned char *input, int sizeInput)
{
// Read and interpret controller response.
//
// (C) 1997-2003 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.
//
    int k;
    DWORD ReceivedLength;
    BOOL ReadDone;
    int bytes{};
    int i{};
    int l{};
    int ReturnLength{};
    int status{};
    bool failed{};
    //unsigned char CheckSum;
    char ReturnText[255];

   failed = true;

// Handle live communications port.
   if (port != NOCOM)
   {
       //	Set expected transfer length.
       switch (command)
       {
       case MFJDRV_NOCOMMAND :
           return 0;
       case MFJDRV_DUMPINPUT :
           bytes = MFJDRV_DUMPLENGTH;
           break;
       case MFJDRV_GETVERSION :
           bytes = 5;
           break;
       default:
           bytes = 4;
           break;
       }
       // Prepare filling of input buffer.
       gCheckLong = 0L;
       input[0] = '\0';
       l = sprintf(ReturnText, "Input: ");
       for (i=1, ReadDone=FALSE; (!ReadDone) && input[0]!=MFJDRV_ACK && input[0]!=MFJDRV_NAK; i++)
       {
           wait_seconds(0.1);
           ReadDone = ReadFile(hCom, input, (DWORD)bytes, &ReceivedLength, NULL);
           if (i%10 == 1 && l < 40)
           {
               sprintf(&ReturnText[l++], ".");
           }
           for (k=0; k<(int)ReceivedLength && k<(sizeof(ReturnText)-l-4)/2; k++)
           {
               sprintf(&ReturnText[l+2*k], "%02X\n", input[k]);
           }
       }
       // remove this if I don't want the input commands to show up
       test_log(ReturnText, false);
       if (ReadDone)
       {
           if (ReceivedLength < (DWORD)bytes)
               status = -1;
           else if (input[0] == MFJDRV_ACK)
               status = input[2];	// Box understood something.
           else if (input[0] == MFJDRV_NAK)
               status = -input[0];	// Box did not understand.
           else
               status = -2;						// Box mumbled if anything.
       }
       else
       {
           status = -103;						// Read failed.
           test_log("Read failed!", false);
       }
       ReturnLength = ReceivedLength;
       if (status < 0) goto WrapUp;
       for (i=1; i<bytes; i++) gCheckLong += input[i];
       if (command == MFJDRV_GETVERSION) // Retrieve version if we asked.
       {
           sprintf(ReturnText, "Version is reported as %d.\n", input[bytes-1]);
           test_log(ReturnText, false);
       }
       //CheckSum = (unsigned char)(gCheckLong & 0xFF);
       failed = false;

   }
// Wrap up response processing.
WrapUp:
    if (failed) ReturnLength = -sizeInput - ReturnLength - 1;
    // Return length of response we've built.
    return ReturnLength;
}

void JetDrive::send_to_jet_drive(int port, int command, unsigned char *output, int lengthOutput)
{
    // Send command sequence to controller.
    //
    // (C) 1997-2000 MicroFab Technologies, Inc., Plano, TX, USA.
    // All rights reserved.
    //
    int status;

    if (command == MFJDRV_NOCOMMAND) return;
    wait_seconds(0.05);
    status = 0;
    if (port != NOCOM)
    {
        DWORD SentLength = 0;
        BOOL WriteDone;
        WriteDone = WriteFile(hCom, output, (DWORD)lengthOutput, &SentLength, NULL);
        if (!WriteDone) status = -104;							// Write failed.
        if (SentLength < (DWORD)lengthOutput) status -= 1000;   // Write incomplete.
        if (status < 0) status = 0x9E;
    }
}

void JetDrive::send_command(int port, int command, float waitTimeSeconds)
{
    unsigned char jetcommands[128];
    unsigned char jetdriveinput[128];
    unsigned int commandlength = 0;

    commandlength = build_command(command, jetcommands);
    send_to_jet_drive(port, command, jetcommands, commandlength);
    wait_seconds(waitTimeSeconds);
    get_from_jet_drive(port, command, jetdriveinput, commandlength);
}

int JetDrive::initialize_jet_drive()
{
    char JetPort[20] = "";
    DWORD InQueue = 256;
    DWORD OutQueue = 256;
    long OpenError;
    DCB useDCB;
    char s[80];

    // Open COM port.
    sprintf(JetPort, "COM%d", mJetDrv+1);
    // Open communications.

    if (mJetDrv != NOCOM)
    {
        hCom = CreateFile(JetPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hCom == NULL) //TODO - DOESN'T RECOGNIZE INVALID_HANDLE_VALUE, SAYS NO SUCH VALUE
        {
            LPVOID lpMsgBuf;
            OpenError = GetLastError();
            //str: fmtJetCommCallFailedWithError
            sprintf(s, fmtCommError, "CreateFile", OpenError);
            test_log(s, true);
            if (FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, OpenError,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR) &lpMsgBuf, 0, NULL) > 0)
            {
                strncpy(s, (const char*)lpMsgBuf, sizeof(s)-1);
                s[sizeof(s)-1] = '\0';
                test_log(s, false);
                delete lpMsgBuf;
            }
            hCom = noCom;
            mJetDrv = NOCOM;
            return 1;
        }
        SetupComm(hCom, InQueue, OutQueue);
        if (GetCommState(hCom, &useDCB))
        {
            //str: fmtJetSizeOfHDCBIsBytes
            sprintf(s, "Size of DCB is %d bytes.\n", sizeof(useDCB));
            test_log(s, false);
            useDCB.BaudRate = 9600;
            useDCB.ByteSize = 8;
            useDCB.Parity = NOPARITY;
            useDCB.StopBits = ONESTOPBIT;
            useDCB.fBinary = 0x1;
            //str: strJetCallingSetCommState
            test_log("Calling SetCommState now.\n", false);
            if (!SetCommState(hCom, &useDCB))
            {
                OpenError = GetLastError();
                //str: fmtJetCommCallFailedWithError
                sprintf(s, fmtCommError, "SetCommState", OpenError);
                test_log(s, true);
                CloseHandle(hCom);
                hCom = noCom;
                mJetDrv = NOCOM;
                return 1;
            }
        }
        else
        {
            OpenError = GetLastError();
            //str: fmtJetCommCallFailedWithError
            sprintf(s, fmtCommError, "GetCommState", OpenError);
            test_log(s, true);
            CloseHandle(hCom);
            hCom = noCom;
            mJetDrv = NOCOM;
            return 1;
        }

        // Start-up for MicroJet III.
        if (gDreamController && !gMultiChannel && !gBreadboardOne)
        {
            char QCmd[] = "Q", XCmd[] = "X2000";
            char Dummy[] = "Q - This is a dummy response. BEEP>";
            //              Q?Invalid command or format?      >
            int l1 = strlen(Dummy);
            int l2 = 49 - l1;
            int lQ = strlen(QCmd);
            int lX = strlen(XCmd);
            char WriteCmd[20];
            int iCmd;
            DWORD Length;
            DWORD OneLength;
            DWORD ReadLength;
            DWORD SentLength;
            DWORD DOne = (DWORD)1L;
            BOOL ReadDone;
            BOOL WriteDone;
            // Stop firmware program, in case it is running.
            Length = (DWORD)lQ;
            if (mJetDrv != NOCOM)
                WriteDone = WriteFile(hCom, (LPCVOID)QCmd, Length, &SentLength, NULL); // SEND Q Command
            else
            {
                WriteDone = TRUE;
                SentLength = Length;
            }
            test_log(QCmd, false);
            if (WriteDone && SentLength == Length)
            {
                wait_seconds(0.5);
                // Read response to Q command (35 or 49 bytes).
                if (mJetDrv != NOCOM)
                {
                    Length = 0;
                    ReadDone = ReadFile(hCom, s, (DWORD)l1, &Length, NULL);
                    if (!ReadDone) Length = 0;
                    s[Length] = '\0';
                }
                else
                {
                    ReadDone = TRUE;
                    Length = (DWORD)l1;
                    strcpy(s, Dummy);
                }
                if (strstr(s, "MFJET32") != NULL)
                {
                    Length = (DWORD)(l1 + 2);
                    gVs6Kludge = true;
                    wait_seconds(2.0);
                    PurgeComm(hCom, PURGE_RXCLEAR);
                }
                if (ReadDone && Length == (DWORD)l1)
                {
                    if (strncmp(s, QCmd, lQ) != 0)
                    {		// Always false for Dummy.
                        ReadDone = ReadFile(hCom, &s[Length], (DWORD)l2, &ReadLength, NULL);
                        if (ReadDone && ReadLength >= (DWORD)l2)
                        {
                            Length += ReadLength;
                        }
                        wait_seconds(3.0);	// Wait some after "Q" failed.
                    }
                    s[Length] = '\0';
                }
                if (ReadDone)
                {
                    test_log(s, false);
                }
                else
                {
                    if (mJetDrv != NOCOM) CloseHandle(hCom);
                    hCom = noCom;
                    mJetDrv = NOCOM;
                    return 1;
                }
                // Start firmware program (again).
                Length = (DWORD)lX;
                if (mJetDrv != NOCOM)
                {
                    SentLength = 0L;
                    ReadLength = 0L;
                    for (iCmd=0; iCmd<lX; iCmd++)
                    {
                        if (mJetDrv != NOCOM)
                        {
                            WriteDone = WriteFile(hCom, &XCmd[iCmd], DOne, &OneLength, NULL);
                        }
                        else
                        {
                            WriteDone = TRUE;
                            OneLength = DOne;
                        }
                        if (!WriteDone || OneLength != DOne)
                        {
                            SentLength = (DWORD)iCmd;
                            break;
                        }
                        SentLength = (DWORD)(iCmd + 1);
                        wait_seconds(0.1);
                        if (mJetDrv != NOCOM)
                        {
                            ReadDone = ReadFile(hCom, &s[iCmd], DOne, &OneLength, NULL);
                        }
                        else
                        {
                            ReadDone = TRUE;
                            OneLength = DOne;
                            s[iCmd] = XCmd[iCmd];
                        }
                        if (!ReadDone || OneLength != DOne)
                        {
                            ReadLength = (DWORD)iCmd;
                            s[ReadLength] = '\0';
                            break;
                        }
                        ReadLength = (DWORD)(iCmd + 1);
                        s[ReadLength] = '\0';
                    }
                    if (ReadLength == (DWORD)lX)
                    {
                        if (mJetDrv != NOCOM && !gVs6Kludge)
                        {
                            ReadDone = ReadFile(hCom, &s[lX], (DWORD)2L, &OneLength, NULL);
                        }
                        else
                        {
                            ReadDone = TRUE;
                            strcpy(&s[lX], "\r\n");
                            OneLength = (DWORD)2L;
                        }
                        if (ReadDone)
                        {
                            ReadLength += OneLength;
                        }
                        s[ReadLength] = '\0';
                    }
                }
                else
                {
                    SentLength = Length;
                    ReadLength = Length + (DWORD)2L;
                    sprintf(s, "%s\r\n", XCmd);
                }
                strcpy(WriteCmd, XCmd);
                WriteCmd[SentLength] = '\0';
                test_log(WriteCmd, false);
                if (ReadLength != Length && SentLength == ReadLength)
                {
                    if (mJetDrv != NOCOM) CloseHandle(hCom);
                    hCom = noCom;
                    mJetDrv = NOCOM;
                    return 1;
                }
                if (SentLength == (DWORD)lX)
                {
                    // Wait a little for the start-up to complete.
                    wait_seconds(1.0);
                }
                else
                {
                    // Start command did not get out the door, close down.
                    if (mJetDrv != NOCOM) CloseHandle(hCom);
                    hCom = noCom;
                    mJetDrv = NOCOM;
                    return 1;
                }
            }
            else
            {
            // Stop command did not get out the door, close down.
                if (mJetDrv != NOCOM) CloseHandle(hCom);
                hCom = noCom;
                mJetDrv = NOCOM;
            return 1;
            }
        }
    }

    unsigned int port = mJetDrv + 1;

    // reads data to set from mJetSettings
    // sets the given command
    // Waits the specified amount of time until it reads the input back from the JetDrive
    send_command(port, MFJDRV_RESET, 3.0f);                   // 1.) Soft Reset
    send_command(port, MFJDRV_GETVERSION, defaultWaitTime);   // 2.) Version
                                                              // 3.) Get Number of Channels (Multi-channel Only) don't need to do this
    send_command(port, MFJDRV_PULSE, 0.55f);                  // 4.) Pulse Wave Form
    send_command(port, MFJDRV_CONTMODE, defaultWaitTime);     // 5.) Trigger Mode
    send_command(port, MFJDRV_DROPS, defaultWaitTime);        // 6.) Number of Drops/Trigger
    send_command(port, MFJDRV_FULLFREQ, defaultWaitTime);     // 7.) Frequency
    send_command(port, MFJDRV_STROBEDIV, defaultWaitTime);    // 8.) Strobe Divider
    send_command(port, MFJDRV_STROBEENABLE, defaultWaitTime); // 9.) Strobe Enable
    send_command(port, MFJDRV_STROBEDELAY, defaultWaitTime);  // 10.) Strobe Delay
    send_command(port, MFJDRV_SOURCE, defaultWaitTime);       // 11.) Trigger Source

    test_log("Connected to JetDrive", true);

    return 0;
}

void JetDrive::set_continuous_jetting()
{
    mJetSettings->fMode = 1;
    send_command(mJetDrv+1, MFJDRV_CONTMODE, defaultWaitTime);
}

void JetDrive::set_single_jetting()
{
    mJetSettings->fMode = 0;
    send_command(mJetDrv+1, MFJDRV_CONTMODE, defaultWaitTime);
}

void JetDrive::set_continuous_mode_frequency(long frequency_Hz)
{
    mJetSettings->fFrequency = frequency_Hz;
    send_command(mJetDrv+1, MFJDRV_FULLFREQ, defaultWaitTime);
}

void JetDrive::set_external_trigger()
{
    mJetSettings->fSource = 1;
    send_command(mJetDrv+1, MFJDRV_CONTMODE, defaultWaitTime);
}

void JetDrive::set_internal_trigger()
{
    mJetSettings->fSource = 0;
    send_command(mJetDrv+1, MFJDRV_CONTMODE, defaultWaitTime);
}

void JetDrive::set_echo_and_dwell_voltage(short echoVoltage_Volts, short dwellVoltage_Volts)
{
    // MAKE SURE THAT NOZZLE IS NOT JETTING BEFORE SENDING THIS COMMAND JUST TO BE SAFE
    if (mJetSettings->fUEcho != echoVoltage_Volts || mJetSettings->fUDwell != dwellVoltage_Volts)
    {
        mJetSettings->fUEcho = echoVoltage_Volts;
        mJetSettings->fUDwell = dwellVoltage_Volts;
        send_command(mJetDrv+1, MFJDRV_PULSE, 0.55f);
    }
}

void JetDrive::start_continuous_jetting()
{
    if (mJetSettings->fMode != 0) set_continuous_jetting();
    if (mJetSettings->fSource != 0) set_internal_trigger();
    send_command(mJetDrv+1, MFJDRV_SOFTTRIGGER, defaultWaitTime);
}

void JetDrive::stop_continuous_jetting()
{
    set_single_jetting(); // this is used to stop continuous jetting
}

void JetDrive::enable_strobe()
{
    if (mJetSettings->fStrobeEnable != 1)
    {
        mJetSettings->fStrobeEnable = 1;
        send_command(mJetDrv+1, MFJDRV_STROBEENABLE, defaultWaitTime);
    }
}

void JetDrive::disable_strobe()
{
    if (mJetSettings->fStrobeEnable != 0)
    {
        mJetSettings->fStrobeEnable = 0;
        send_command(mJetDrv+1, MFJDRV_STROBEENABLE, defaultWaitTime);
    }
}

void JetDrive::set_strobe_delay(short strobeDelay_microseconds)
{
    if (mJetSettings->fStrobeDelay != strobeDelay_microseconds)
    {
        mJetSettings->fStrobeDelay = strobeDelay_microseconds;
        send_command(mJetDrv+1, MFJDRV_STROBEDELAY, defaultWaitTime);
    }
}
