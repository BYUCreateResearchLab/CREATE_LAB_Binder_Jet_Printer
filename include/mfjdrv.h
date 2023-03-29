#ifndef MFJDRV_H
#define MFJDRV_H

#include <QByteArray>

namespace JetDrive
{

struct Waveform
{
    double fTRise        =   3.0; // Rise Time 1 (µs)
    double fTDwell       =  20.0; // Dwell Time  (µs)
    double fTFall        =   3.0; // Fall Time   (µs)
    double fTEcho        =  40.0; // Echo Time   (µs)
    double fTFinal       =   3.0; // Rise Time 2 (µs)
    double fTDelay       =   0.0;

    short fUIdle         =     0; // Idle Voltage  (V)
    short fUDwell        =    20; // Dwell Voltage (V)
    short fUEcho         =   -20; // Echo Voltage  (V)
};

struct Settings
{
    Waveform waveform;
    long fFrequency      = 1000L; // Jetting Frequency (Hz)
    short fUGain         =   225;
    short fMode          =     0; // fmode is to set continuous jetting mode (1 to enable continuous jetting, 0 to disable)
    short fSource        =     1; // fSource is for setting the trigger source (1 for external TTL trigger, 0 for internal trigger)
    short fDrops         =     1; // number of drops per single trigger
    short fStrobeDelay   =     0; // sets strobe delay in microseconds
    short fStrobeDiv     =     1; // divider for the stobe (1 means the strobe flashes every pulse, 2 means the strobe flashes every other jetting pulse)
    short fStrobeEnable  =     1;
    short fDebugSwitch   =     0;
    short fDebugValue    =     0;
    short fChannelGroup  =     0;
    short fContIsStarted =     0;
    short fChannelOn     =     0;
};

enum class CMD
{
    NOCOMMAND    = 0x00,
    RESET        = 0x01,
    POLLSTATUS   = 0x02,
    DROPS        = 0x03,
    CONTMODE     = 0x04,
    FREQUENCY    = 0x05,
    PULSE        = 0x06,
    STROBEDIV    = 0x07,
    SOURCE       = 0x08,
    SOFTTRIGGER  = 0x09,
    STROBEENABLE = 0x10,
    LOWFREQ      = 0x11,
    FULLFREQ     = 0x12,
    STROBEDELAY  = 0x13,
    DUMPINPUT    = 0x60,
    DEBUG        = 0x61,
    POKE         = 0xEF,
    GETVERSION   = 0xF0,

    MULTITRIGGER = 0x0D,
    EXTERNENABLE = 0x0C,
    EDITCHANNEL  = 0x0E
};

class CommandBuilder
{

public:

    explicit CommandBuilder();
    const QByteArray build(CMD command,
                                   Settings &m_jetParams);

private:

    const bool gDreamController {true};
    const bool gMultiChannel {false};
    const bool gBreadboardOne {false};

    long gCheckLong {0L};
    const int gFirmVersion {0};
    const short gExternEnable {0};
    const bool gVs6Kludge {false};
    const double gRateLimit {30.0};
};

inline short response_size(CMD command)
{
    switch (command)
    {
    default: return 4;
    case CMD::GETVERSION: return 5;
    case CMD::MULTITRIGGER: return 5;
    case CMD::DUMPINPUT: return 27;
    }
}

}


#endif // MFJDRV_H
