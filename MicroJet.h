#pragma once

#include <windows.h>
#include <ctype.h>
#include <sys/timeb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#define COM1 0
#define COM2 1
#define COM4 3

// Command indicators
#define MFJDRV_NOCOMMAND		0x00
#define MFJDRV_RESET				0x01
#define MFJDRV_POLLSTATUS		0x02
#define MFJDRV_DROPS				0x03
#define MFJDRV_CONTMODE			0x04
#define MFJDRV_FREQUENCY		0x05
#define MFJDRV_PULSE				0x06
#define MFJDRV_STROBEDIV		0x07
#define MFJDRV_SOURCE			0x08
#define MFJDRV_SOFTTRIGGER		0x09
#define MFJDRV_STROBEENABLE	0x10
#define MFJDRV_LOWFREQ			0x11
#define MFJDRV_FULLFREQ			0x12
#define MFJDRV_STROBEDELAY		0x13
#define MFJDRV_DUMPINPUT		0x60
#define MFJDRV_DEBUG				0x61
#define MFJDRV_POKE				0xEF
#define MFJDRV_GETVERSION		0xF0

#define MFJDRV_FREQBASE 		625000.0
#define MFJDRV_ACK				0x06
#define MFJDRV_NAK				0x15

#define MFJDRV_DUMPLENGTH		29


#define MFJDRV_MULTITRIGGER 0x0D
#define MFJDRV_EXTERNENABLE 0x0C
#define MFJDRV_EDITCHANNEL 0x0E
// I don't think these 3 are the right commands, but I'm not sure it matter for our JetDrive...



#define gDreamController true
#define gMultiChannel false
#define gBreadboardOne false 
//DOES THIS WORK??


class MicroJet {
    // Data structure for microjet operating and status parameters.
    //
    // H.-J.Trost - 010103   Use only a default constructor.
    // H.-J.Trost - 000703
    //
public:
    MicroJet();
    ~MicroJet();
    double fTRise, fTDwell, fTFall, fTEcho, fTFinal, fTDelay;
    long fFrequency;
    short fUIdle, fUDwell, fUEcho, fUGain,
        fMode, fSource, fDrops, fStrobeDelay, fStrobeDiv, fStrobeEnable,
        fDebugSwitch, fDebugValue, fChannelGroup, fContIsStarted, fChannelOn;
private:
    static short fgNJets;
};

short MicroJet::fgNJets = 0;

// fmode is to set continuous jetting mode (1 to enable continuous jetting, 0 to disable)
// fSource is for setting the trigger source (1 for external TTL trigger, 0 for internal trigger)
// fFrequency is for setting the jetting frequency (in Hz?)

MicroJet::MicroJet() :
    fTRise(3.0), fTDwell(20.0), fTFall(4.0), fTEcho(40.0), fTFinal(3.0),
    fTDelay(0.0), fFrequency(1000L),
    fUIdle(0), fUDwell(60), fUEcho(-60), fUGain(225),
    fMode(1), fSource(0), fDrops(1), fStrobeDelay(0), fStrobeDiv(1),
    fStrobeEnable(1), fDebugSwitch(0), fDebugValue(0), fChannelGroup(0),
    fContIsStarted(0), fChannelOn(0)
{
    // Default constructor.
    //
    // H.-J. Trost - 010103   last mod.
    // H.-J. Trost - 000703
    //
    fgNJets++;
}


MicroJet::~MicroJet()
{
    // Destructor.
    //
    // H.-J. Trost - 000703   last mod.
    // H.-J. Trost - 000703
    //
    fgNJets--;
}


static const char fmtCommError[] = "%s failed with error %ld.\n";
static HANDLE hCom = INVALID_HANDLE_VALUE, noCom = INVALID_HANDLE_VALUE;


static long  gCheckLong = 0L;
static int   gFirmVersion = 0, gNumContMode = 0;
static short gExternEnable = 0;
static bool  gDumpRange = false, gMultiTrigMode = false, gVs6Kludge = false;
// Parameter limits.
static double gRateLimit = 30.0;
static long  gTimeLow = 1L, gTimeHigh = 8191L,
gVoltLow = -500L, gVoltHigh = 500L, gFreqHigh = 20000L;
static short gDropsHigh = 999, gStrobeHigh = 64, gStrobeDelayLow = -500,
gStrobeDelayHigh = 2500;
// Screen parameters.
static int	gTopLine = 2, gTopParams = 7, gRowsParams = 9,
gBottomBlock = 19, gMessageLine = 24,
gPulseTitleCol = 20, gTriggerTitleCol = 50,
gCommandCol = 11, gStartCol = 33, gAloneCol = 47, gDumpCol = 54,
gExitCol = 67, gChanBottom = 15;


#define NOCOM    -1
