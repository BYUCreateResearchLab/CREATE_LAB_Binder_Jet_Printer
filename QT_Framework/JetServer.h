#ifndef JETSERVER_H
#define JETSERVER_H

#endif // JETSERVER_H

//	Drive electronics interface program (excerpts for reference)
//
//      (C) 1997-2003 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.
//

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
#define COM9 8

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

//Check to make sure the max is correct
float max(float a,float b)
{
    if((a) > (b)) {
        return a;
    }
    else {
        return b;
    }
}

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
    MicroJet() ;
      ~MicroJet() ;
      double fTRise, fTDwell, fTFall, fTEcho, fTFinal, fTDelay ;
      long fFrequency ;
      short fUIdle, fUDwell, fUEcho, fUGain,
            fMode, fSource, fDrops, fStrobeDelay, fStrobeDiv, fStrobeEnable,
            fDebugSwitch, fDebugValue, fChannelGroup, fContIsStarted, fChannelOn;
   private:
      static short fgNJets ;
} ;

short MicroJet::fgNJets = 0 ;

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
    fgNJets++ ;
}


MicroJet::~MicroJet()
{
// Destructor.
//
// H.-J. Trost - 000703   last mod.
// H.-J. Trost - 000703
//
    fgNJets-- ;
}


static const char fmtCommError[] = "%s failed with error %ld.\n" ;
HANDLE hCom = INVALID_HANDLE_VALUE, noCom = INVALID_HANDLE_VALUE ;


static long  gCheckLong = 0L ;
static int   gFirmVersion = 0, gNumContMode = 0 ;
static short gExternEnable = 0 ;
static bool  gDumpRange = false, gMultiTrigMode = false, gVs6Kludge = false ;
// Parameter limits.
static double gRateLimit = 30.0 ;
static long  gTimeLow = 1L, gTimeHigh = 8191L,
                 gVoltLow = -500L, gVoltHigh = 500L, gFreqHigh = 20000L ;
static short gDropsHigh = 999, gStrobeHigh = 64, gStrobeDelayLow = -500,
             gStrobeDelayHigh = 2500 ;
// Screen parameters.
static int	gTopLine = 2, gTopParams = 7, gRowsParams = 9,
            gBottomBlock = 19, gMessageLine = 24,
            gPulseTitleCol = 20, gTriggerTitleCol = 50,
            gCommandCol = 11, gStartCol = 33, gAloneCol = 47, gDumpCol = 54,
            gExitCol = 67, gChanBottom = 15 ;


int NOCOM = -1;


// This is me building my own structure for this function.. Put here to be global

MicroJet gjets1;
MicroJet* gJets[1] = {&gjets1};
unsigned int gCJ = 0;


// I made these functions

void WaitSeconds(float seconds) {
    Sleep(seconds * 1000);
}

void TestLog(const char* outputstring, bool status) {
    std::cout << outputstring << std::endl;
}


// Function Declarations
int BuildCommand(int Command, unsigned char *JetCommand)
{
// Build a Stortz command from current parameters.
//
//	(C) 1997-2000 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.
//
    long LongFreq ;
    int CommandLength, i, IntTime ;
   char CommandText[255] ;

    CommandLength = 3 ;
    JetCommand[0] = 'S' ;
    JetCommand[2] = (unsigned char)(Command & 0xFF) ;



    switch (Command) {

        case MFJDRV_NOCOMMAND :
            JetCommand[0] = '\0' ;
            CommandLength = 0 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\n(No command)") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_RESET :
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nReset") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_POLLSTATUS :
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nPoll status") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_DROPS :
            // WAS : if (gDreamController || gJets[gCJ]->fDrops > 255)
            if (gDreamController || gJets[gCJ]->fDrops > 255) {
                JetCommand[3] = (unsigned char)((gJets[gCJ]->fDrops >> 8) &
                                                      0x00FF) ;
                JetCommand[4] = (unsigned char)(gJets[gCJ]->fDrops & 0x00FF) ;
                CommandLength = 5 ;
            }
            else {
                JetCommand[3] = (unsigned char)(gJets[gCJ]->fDrops & 0x00FF) ;
                CommandLength = 4 ;
            }
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet drops/trigger to %d.",
                  gJets[gCJ]->fDrops) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_CONTMODE :
            JetCommand[3] = (unsigned char)(gJets[gCJ]->fMode ? 0x01 : 0x00) ;
            CommandLength = 4 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet trigger mode to %s.",
                  gJets[gCJ]->fMode ? "continuous" : "single") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_FREQUENCY :
            if (gJets[gCJ]->fFrequency < 1L) gJets[gCJ]->fFrequency = 1L ;
            LongFreq = (long)(MFJDRV_FREQBASE / (double)gJets[gCJ]->fFrequency -
                                0.5) ;
            if (LongFreq < 1L) LongFreq = 1L ;
         if (gFirmVersion < 40 && LongFreq > 4095L) LongFreq = 4095L ;
            if (LongFreq <= 4095L) {
                JetCommand[3] = (unsigned char)((LongFreq >> 8) & 0x000000FF) ;
                JetCommand[4] = (unsigned char)(LongFreq & 0x000000FF) ;
                CommandLength = 5 ;
            }
            else {
                JetCommand[2] = MFJDRV_LOWFREQ ;
                JetCommand[3] = (unsigned char)(gJets[gCJ]->fFrequency &
                                                      0x000000FF) ;
                CommandLength = 4 ;
            }
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet frequency to %ld Hz (divider = %ld).",
                  gJets[gCJ]->fFrequency, LongFreq) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_PULSE :
            IntTime = (int)(gJets[gCJ]->fTDwell * 10.0 + 0.001) ;
            JetCommand[5] = (unsigned char)((IntTime >> 8) & 0x00FF) ;
            JetCommand[6] = (unsigned char)(IntTime & 0x00FF) ;
            IntTime = (int)(gJets[gCJ]->fTEcho * 10.0 + 0.001) ;
            JetCommand[8] = (unsigned char)((IntTime >> 8) & 0x00FF) ;
            JetCommand[9] = (unsigned char)(IntTime & 0x00FF) ;
            if (gDreamController ||
             abs(gJets[gCJ]->fUIdle) > 100 ||
                 abs(gJets[gCJ]->fUDwell) > 100 ||
                 abs(gJets[gCJ]->fUEcho) > 100 ||
                 gJets[gCJ]->fTRise > 0.11 ||
                 gJets[gCJ]->fTFall > 0.11 ||
                 gJets[gCJ]->fTFinal > 0.11) {
            JetCommand[ 3] = 0x00 ;
            JetCommand[ 4] = 0x00 ;
            JetCommand[ 7] = 0x00 ;
                JetCommand[10] = (signed char)((gJets[gCJ]->fUIdle >> 8) & 0x00FF) ;
                JetCommand[11] = (signed char)(gJets[gCJ]->fUIdle & 0x00FF) ;
                JetCommand[12] = (signed char)((gJets[gCJ]->fUDwell >> 8) &
                                                     0x00FF) ;
                JetCommand[13] = (signed char)(gJets[gCJ]->fUDwell & 0x00FF) ;
                JetCommand[14] = (signed char)((gJets[gCJ]->fUEcho >> 8) & 0x00FF) ;
                JetCommand[15] = (signed char)(gJets[gCJ]->fUEcho & 0x00FF) ;
            int minTime = (int)(fabs(((double)
                                     (gJets[gCJ]->fUDwell-gJets[gCJ]->fUIdle) /
                                  gRateLimit)) * 10.0) + 10 ;
                IntTime = max((int)(gJets[gCJ]->fTRise * 10.0 + 0.001), minTime) ;
                JetCommand[16] = (unsigned char)((IntTime >> 8) & 0x00FF) ;
                JetCommand[17] = (unsigned char)(IntTime & 0x00FF) ;
            minTime = (int)(fabs(((double)
                                     (gJets[gCJ]->fUEcho-gJets[gCJ]->fUDwell) /
                                  gRateLimit)) * 10.0) + 10 ;
                IntTime = max((int)(gJets[gCJ]->fTFall * 10.0 + 0.001), minTime) ;
                JetCommand[18] = (unsigned char)((IntTime >> 8) & 0x00FF) ;
                JetCommand[19] = (unsigned char)(IntTime & 0x00FF) ;
            minTime = (int)(fabs(((double)
                                     (gJets[gCJ]->fUIdle-gJets[gCJ]->fUEcho) /
                                  gRateLimit)) * 10.0) + 10 ;
                IntTime = max((int)(gJets[gCJ]->fTFinal * 10.0 + 0.001), minTime) ;
                JetCommand[20] = (unsigned char)((IntTime >> 8) & 0x00FF) ;
                JetCommand[21] = (unsigned char)(IntTime & 0x00FF) ;
                CommandLength = 22 ;
            }
         else {
                JetCommand[3] = (signed char)(gJets[gCJ]->fUIdle & 0x00FF) ;
                JetCommand[4] = (signed char)(gJets[gCJ]->fUDwell & 0x00FF) ;
                if ((gJets[gCJ]->fUEcho - gJets[gCJ]->fUIdle)*
                 (gJets[gCJ]->fUDwell - gJets[gCJ]->fUIdle) < 0) {
                    JetCommand[7] = (signed char)(gJets[gCJ]->fUEcho & 0x00FF) ;
                    CommandLength = 10 ;
                }
                else
                    CommandLength = 7 ;
            }
#ifdef MFDRV_DEBUG
         sprintf(CommandText,
          "\nSet waveform to U=%d/%d/%d V, t=%.1lf/%.1lf/%.1lf/%.1lf/%.1lf %s.",
                  gJets[gCJ]->fUIdle, gJets[gCJ]->fUDwell, gJets[gCJ]->fUEcho,
                 gJets[gCJ]->fTRise, gJets[gCJ]->fTDwell, gJets[gCJ]->fTFall,
                 gJets[gCJ]->fTEcho, gJets[gCJ]->fTFinal, gMicroS) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_STROBEDIV :
            JetCommand[3] = (unsigned char)(gJets[gCJ]->fStrobeDiv & 0x00FF) ;
            CommandLength = 4 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet strobe divider to %d.",
                  gJets[gCJ]->fStrobeDiv) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_SOURCE :
            if (gMultiChannel) {
                JetCommand[3] = (unsigned char)(gJets[gCJ]->fChannelOn ? 0x01 :
                                                                                        0x00) ;
#ifdef MFDRV_DEBUG
            sprintf(CommandText, "\n%sclude channel %d %s multi-trigger set.",
                      gJets[gCJ]->fChannelOn ? "In" : "Ex", gCJ+1,
                    gJets[gCJ]->fChannelOn ? "into" : "from") ;
#endif
            }
         else {
                JetCommand[3] = (unsigned char)(gJets[gCJ]->fSource ? 0x01 : 0x00) ;
#ifdef MFDRV_DEBUG
            sprintf(CommandText, "\nSet trigger source to %s.",
                      gJets[gCJ]->fSource ? "External" : "PC Trig.") ;
#endif  //  ifdef MFDRV_DEBUG
            }
            CommandLength = 4 ;
            break ;

        case MFJDRV_SOFTTRIGGER :
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nTrigger output.") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_MULTITRIGGER :
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nMulti-trigger output.") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_EXTERNENABLE :
        if (!gMultiChannel) JetCommand[2] = MFJDRV_SOURCE ;
            JetCommand[3] = (unsigned char)(gExternEnable ? 0x01 : 0x00) ;
            CommandLength = 4 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\n%sable external trigger.",
                  gExternEnable ? "En" : "Dis") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_EDITCHANNEL :
        if (gMultiChannel) {
            JetCommand[3] = (unsigned char)(gCJ & 0xFF) ;
             CommandLength = 4 ;
#ifdef MFDRV_DEBUG
             sprintf(CommandText, "\nEdit channel %d.", gCJ+1) ;
#endif  //  ifdef MFDRV_DEBUG
            }
         else
            CommandLength = 0 ;
            break ;

        case MFJDRV_STROBEENABLE :
            JetCommand[3] = (unsigned char)(gJets[gCJ]->fStrobeEnable ? 0x01 :
                                                                                        0x00) ;
            CommandLength = 4 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\n%sable strobe.",
                  gJets[gCJ]->fStrobeEnable ? "En" : "Dis") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_LOWFREQ :
            if (gJets[gCJ]->fFrequency >= 256L || gFirmVersion < 40)
                CommandLength = -1 ;
            else {
                JetCommand[3] = (unsigned char)(gJets[gCJ]->fFrequency &
                                                      0x000000FF) ;
                CommandLength = 4 ;
            }
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet low frequency to %ld Hz.",
                  gJets[gCJ]->fFrequency) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_FULLFREQ :	// new with DREAM design
            if (gJets[gCJ]->fFrequency >= 65536L)
                CommandLength = -1 ;
            else {
                JetCommand[3] = (unsigned char)((gJets[gCJ]->fFrequency >> 8) &
                                                      0x000000FF) ;
                JetCommand[4] = (unsigned char)(gJets[gCJ]->fFrequency &
                                                      0x000000FF) ;
                CommandLength = 5 ;
            }
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet frequency to %ld Hz (vs.5).",
                  gJets[gCJ]->fFrequency) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_STROBEDELAY :	// new with DREAM design
        JetCommand[3] = 0x01 ;	// Keep potentiometer on at all times.
            JetCommand[4] = (unsigned char)((gJets[gCJ]->fStrobeDelay >> 8) &
                                                  0x00FF) ;
            JetCommand[5] = (unsigned char)(gJets[gCJ]->fStrobeDelay & 0x00FF) ;
            CommandLength = 6 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nSet strobe delay to %d %s.",
                  gJets[gCJ]->fStrobeDelay, gMicroS) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_DUMPINPUT :	// new with DREAM design
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nRequest dump of settings.") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_DEBUG :	// new with DREAM design
            JetCommand[3] = (unsigned char)(gJets[gCJ]->fDebugSwitch & 0xFF) ;
         JetCommand[4] = (unsigned char)(gJets[gCJ]->fDebugValue & 0xFF) ;
         CommandLength = 5 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nDebug for switch %02Xh value %02Xh.",
                  JetCommand[3], JetCommand[4]) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_POKE :
            JetCommand[3] = 0x40 ;
            JetCommand[4] = 0x07 ;
            JetCommand[5] = 0x0D ;
            CommandLength = 6 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nPoke") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        case MFJDRV_GETVERSION :
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nGet version") ;
#endif  //  ifdef MFDRV_DEBUG
            break ;

        default:
            CommandLength = -1 ;
#ifdef MFDRV_DEBUG
         sprintf(CommandText, "\nCommand not recognized: %04Xh", Command) ;
#endif  //  ifdef MFDRV_DEBUG
            break ;
    }

   if (CommandLength > 0) {
#ifdef MFDRV_DEBUG
       TestLog(CommandText, true) ;
#endif  //  ifdef MFDRV_DEBUG
        if (CommandLength > 1) {
            JetCommand[1] = (unsigned char)(CommandLength - 1) ;
            gCheckLong = 0L ;
            for (i=1; i<CommandLength; i++)
                gCheckLong += (long)JetCommand[i] ;
            JetCommand[CommandLength++] = (unsigned char)(gCheckLong &
                                                                     0x000000FF) ;
        }
        else
            CommandLength = 0 ;
    }
    return CommandLength ;
}

int GetJetDrv(int port, int Command, unsigned char *Input, int SizeInput)
{
// Read and interpret controller response.
//
//	(C) 1997-2003 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.
//
    int k ;
    DWORD ReceivedLength ;
   BOOL ReadDone ;
    int bytes, i, l, ReturnLength, status ;
   bool failed ;
   unsigned char CheckSum ;
   char ReturnText[255] ;

   failed = true ;

// Handle live communications port.
    if (port != NOCOM) {
        //	Set expected transfer length.
      switch (Command) {
            case MFJDRV_NOCOMMAND :
                return 0 ;
          case MFJDRV_DUMPINPUT :
            bytes = MFJDRV_DUMPLENGTH ;
               break ;
            case MFJDRV_GETVERSION :
            bytes = 5 ;
            break ;
         default:
            bytes = 4 ;
              break ;
      }
      // Prepare filling of input buffer.
        gCheckLong = 0L ;
        Input[0] = '\0' ;
        l = sprintf(ReturnText, "Input: ") ;
        for (i=1, ReadDone=FALSE; (!ReadDone) &&
                                Input[0]!=MFJDRV_ACK &&
                                Input[0]!=MFJDRV_NAK; i++) {
            WaitSeconds(0.1) ;
            ReadDone = ReadFile(hCom, Input, (DWORD)bytes, &ReceivedLength, NULL) ;
            if (i%10 == 1 && l < 40) sprintf(&ReturnText[l++], ".") ;
            for (k=0; k<(int)ReceivedLength && k<(sizeof(ReturnText)-l-4)/2; k++)
                sprintf(&ReturnText[l+2*k], "%02X\n", Input[k]) ;
        }
        TestLog(ReturnText, false) ;
        if (ReadDone) {
            if (ReceivedLength < (DWORD)bytes)
                status = -1 ;
            else if (Input[0] == MFJDRV_ACK)
                status = Input[2] ;	// Box understood something.
            else if (Input[0] == MFJDRV_NAK)
                status = -Input[0] ;	// Box did not understand.
            else
                status = -2 ;						// Box mumbled if anything.
        }
        else
            status = -103 ;						// Read failed.
      ReturnLength = ReceivedLength ;
      if (status < 0) goto WrapUp ;
      for (i=1; i<bytes; i++) gCheckLong += Input[i] ;
        if (Command == MFJDRV_GETVERSION) {	// Retrieve version if we asked.
         sprintf(ReturnText, "Version is reported as %d.\n", Input[bytes-1]) ;
         TestLog(ReturnText, false) ;
        }
        CheckSum = (unsigned char)(gCheckLong & 0xFF) ;
      failed = false ;

   }
// Wrap up response processing.
WrapUp:
    if (failed) ReturnLength = - SizeInput - ReturnLength - 1 ;
    // Return length of response we've built.
    return ReturnLength ;
}



void SendJetDrv(int port, int Command,
                     unsigned char *Output, int LengthOutput)
{
// Send command sequence to controller.
//
//	(C) 1997-2000 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.
//
    int status ;

    if (Command == MFJDRV_NOCOMMAND) return ;
   WaitSeconds(0.05) ;
    status = 0 ;
    if (port != NOCOM) {
        DWORD SentLength = 0 ;
      BOOL WriteDone ;
        WriteDone = WriteFile(hCom, Output, (DWORD)LengthOutput,
                                 &SentLength, NULL) ;
      if (!WriteDone) status = -104 ;							// Write failed.
        if (SentLength < (DWORD)LengthOutput) status -= 1000 ;// Write incomplete.
      if (status < 0) status = 0x9E ;
   }
}


void SendCommand(int port, int Command, float waitTimeSeconds)
{
    unsigned char jetcommands[128];
    unsigned char jetdriveinput[128];
    unsigned int commandlength = 0;

    commandlength = BuildCommand(Command, jetcommands);
    SendJetDrv(port, Command, jetcommands, commandlength);
    WaitSeconds(waitTimeSeconds);
    GetJetDrv(port, Command, jetdriveinput, commandlength);
}


// A section showing how a dump response is picked apart and compared to the expected values.
//
// A second jet data structure gDumpJets has been created just like the regular gJets structure,
// and the dump data are filled into it.  Then a simple comparison of data members can be performed.
//
// Note that the Dump command will not return any information on the sine nor arbitrary lines
// waveforms.
//
// This code section could be inserted in the GetJetDrv function  just before the lines
//          // Wrap up response processing.
//          WrapUp:

// Check on dump return.

/*
   if (Command == MFJDRV_DUMPINPUT && ReturnLength >= MFJDRV_DUMPLENGTH) {
      double AddForInt ;
      AddForInt = gDreamController ? 0.0001 : 0.5001 ;
      DumpBits   = 0L ;
      gDumpJets[gCJ]->fUIdle  =  (double)((Input[ 2] << 8) | Input[ 3]) ;
      DumpBits   |= fabs(gJets[gCJ]->fUIdle - gDumpJets[gCJ]->fUIdle) > 0.001 ?
                                0x00000001L : 0L ;
      // Note: Times come back in full 盜, not 0.1 盜 units.
      gDumpJets[gCJ]->fTRise  = (double)((Input[ 4] << 8) | Input[ 5]) ;
      DumpBits   |= fabs(gJets[gCJ]->fTRise - gDumpJets[gCJ]->fTRise -
                             AddForInt) > 0.51 ?
                                                                            0x00000002L : 0L ;
      gDumpJets[gCJ]->fUDwell =  (double)((Input[ 6] << 8) | Input[ 7]) ;
      DumpBits   |= fabs(gJets[gCJ]->fUDwell - gDumpJets[gCJ]->fUDwell)
                                                  > 0.001 ? 0x00000004L : 0L ;
      gDumpJets[gCJ]->fTDwell = (double)((Input[ 8] << 8) | Input[ 9]) ;
      DumpBits   |= fabs(gJets[gCJ]->fTDwell - gDumpJets[gCJ]->fTDwell -
                             AddForInt) > 0.51 ?
                                                                            0x00000008L : 0L ;
      gDumpJets[gCJ]->fTFall  = (double)((Input[10] << 8) | Input[11]) ;
      DumpBits   |= fabs(gJets[gCJ]->fTFall - gDumpJets[gCJ]->fTFall -
                             AddForInt) > 0.51 ?
                                                                            0x00000010L : 0L ;
      gDumpJets[gCJ]->fUEcho  =  (double)((Input[12] << 8) | Input[13]) ;
      DumpBits   |= fabs(gJets[gCJ]->fUEcho - gDumpJets[gCJ]->fUEcho) > 0.001 ?
                                                                            0x00000020L : 0L ;
      gDumpJets[gCJ]->fTEcho  = (double)((Input[14] << 8) | Input[15]) ;
      DumpBits   |= fabs(gJets[gCJ]->fTEcho - gDumpJets[gCJ]->fTEcho -
                             AddForInt) > 0.51 ?
                                                                            0x00000040L : 0L ;
      gDumpJets[gCJ]->fTFinal = (double)((Input[16] << 8) | Input[17]) ;
      DumpBits   |= fabs(gJets[gCJ]->fTFinal - gDumpJets[gCJ]->fTFinal -
                             AddForInt) > 0.51 ?
                                                                            0x00000080L : 0L ;
      gDumpJets[gCJ]->fDrops  =  (short)((Input[18] << 8) | Input[19]) ;
      DumpBits   |= gJets[gCJ]->fDrops != gDumpJets[gCJ]->fDrops ?
                                                                            0x00000100L : 0L ;
      gDumpJets[gCJ]->fStrobeDiv = (short)Input[20] ;
      DumpBits   |= gJets[gCJ]->fStrobeDiv != gDumpJets[gCJ]->fStrobeDiv ?
                                                                            0x00000200L : 0L ;
      Interval   = gJets[gCJ]->fFrequency != 0L ?
                            (long)(1.0E6 / (double)gJets[gCJ]->fFrequency +
                                 AddForInt) : 0L ;
      PRIFrequency = Interval != 0L ?
                            (long)(1.0E6 / (double)Interval + AddForInt) : 0L ;
        DumpInterval = (long)(((long)Input[21] << 16) |
                                 ((long)Input[22] << 8) | (long)Input[23]) ;
        gDumpJets[gCJ]->fFrequency = DumpInterval != 0 ?
                            (long)(1.0E6 / (double)DumpInterval + AddForInt) : 0L ;
        DumpBits   |= Interval != DumpInterval ?         		0x00000400L : 0L ;
      gDumpJets[gCJ]->fStrobeDelay = (short)((Input[24] << 8) | Input[25]) ;
      DumpBits   |= gJets[gCJ]->fStrobeDelay != gDumpJets[gCJ]->fStrobeDelay ?
                                                                            0x00000800L : 0L ;
        DumpStatus =  (int)Input[26] ;
      gDumpJets[gCJ]->fContIsStarted = (DumpStatus & 0x0001) != 0 ;
      DumpBits   |= gJets[gCJ]->fContIsStarted != gDumpJets[gCJ]->fContIsStarted ?
                                                                            0x00001000L : 0L ;
      gDumpJets[gCJ]->fMode   =  (short)((DumpStatus & 0x0002) ? 1 : 0) ;
      DumpBits   |= gJets[gCJ]->fMode != gDumpJets[gCJ]->fMode ?
                                                                            0x00002000L : 0L ;
      gDumpJets[gCJ]->fSource =  (short)((DumpStatus & 0x0004) ? 1 : 0) ;
      DumpBits   |= gJets[gCJ]->fSource != gDumpJets[gCJ]->fSource ?
                                                                            0x00004000L : 0L ;
      gDumpJets[gCJ]->fStrobeEnable = (short)((DumpStatus & 0x0008) ? 1 : 0) ;
      DumpBits   |= gJets[gCJ]->fStrobeEnable != gDumpJets[gCJ]->fStrobeEnable ?
                                                                            0x00008000L : 0L ;
      DumpFirmVersion = (int)Input[27] ;
      DumpBits   |= gFirmVersion != DumpFirmVersion ?   		0x00010000L : 0L ;
        // Report the locally composed flags always.
      sprintf(ReturnText, "Dump flags word: %08lXh, changed by %08lXh.",
              DumpBits, DumpBits ^ OldDumpBits) ;
      TestLog(ReturnText, true) ;
      OldDumpBits = DumpBits ;
#ifdef MFDRV_DEBUG
        TestLog("Parameter pairs are expected first and found second:", false) ;
      sprintf(ReturnText,
              "Voltages: idle %.1lf %.1lf, dwell %.1lf %.1lf, echo %.1lf %.1lf",
              gJets[gCJ]->fUIdle, gDumpJets[gCJ]->fUIdle,
              gJets[gCJ]->fUDwell, gDumpJets[gCJ]->fUDwell,
              gJets[gCJ]->fUEcho, gDumpJets[gCJ]->fUEcho) ;
      TestLog(ReturnText, false) ;
      sprintf(ReturnText,
      "Times: rise %7.1lf %7.1lf, dwell %7.1lf %7.1lf, fall %7.1lf %7.1lf",
                  gJets[gCJ]->fTRise, gDumpJets[gCJ]->fTRise,
              gJets[gCJ]->fTDwell, gDumpJets[gCJ]->fTDwell,
              gJets[gCJ]->fTFall, gDumpJets[gCJ]->fTFall) ;
        TestLog(ReturnText, false) ;
      sprintf(ReturnText, "       echo %7.1lf %7.1lf, final %7.1lf %7.1lf",
              gJets[gCJ]->fTEcho, gDumpJets[gCJ]->fTEcho,
              gJets[gCJ]->fTFinal, gDumpJets[gCJ]->fTFinal) ;
        TestLog(ReturnText, false) ;
      sprintf(ReturnText, "Drops: %d %d",
              gJets[gCJ]->fDrops, gDumpJets[gCJ]->fDrops) ;
      TestLog(ReturnText, false) ;
      sprintf(ReturnText, "Strobe divider: %d %d",
              gJets[gCJ]->fStrobeDiv, gDumpJets[gCJ]->fStrobeDiv) ;
      TestLog(ReturnText, false) ;
        sprintf(ReturnText,
              "Frequency: %ld %ld, PRI'd frequency %ld, interval %ld %ld",
                  gJets[gCJ]->fFrequency, gDumpJets[gCJ]->fFrequency,
              PRIFrequency, Interval, DumpInterval) ;
        TestLog(ReturnText, false) ;
        sprintf(ReturnText, "Jetting: %s %s",
                              gJets[gCJ]->fContIsStarted ? "on" : "off",
                          gDumpJets[gCJ]->fContIsStarted ? "on" : "off") ;
      TestLog(ReturnText, false) ;
      sprintf(ReturnText, "Mode: %d %d",
              gJets[gCJ]->fMode, gDumpJets[gCJ]->fMode) ;
      TestLog(ReturnText, false) ;
      sprintf(ReturnText, "Source: %d %d",
              gJets[gCJ]->fSource, gDumpJets[gCJ]->fSource) ;
      TestLog(ReturnText, false) ;
      sprintf(ReturnText, "Strobe enable: %d %d",
              gJets[gCJ]->fStrobeEnable, gDumpJets[gCJ]->fStrobeEnable) ;
      TestLog(ReturnText, false) ;
      sprintf(ReturnText, "Firmware version: %d %d",
              gFirmVersion, DumpFirmVersion) ;
      TestLog(ReturnText, false) ;
#endif
   }


   */
