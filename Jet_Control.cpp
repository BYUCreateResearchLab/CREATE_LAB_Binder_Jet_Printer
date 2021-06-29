//	Drive electronics interface program (excerpts for reference)
//
//      (C) 1997-2003 MicroFab Technologies, Inc., Plano, TX, USA.
// All rights reserved.
// Git Push Test

#include "MicroJet_Functions.h"

int main(int argc, char* argv[])
{
    // Main program coordinating all actions.
    //
    //	(C) 1997-2000 MicroFab Technologies, Inc., Plano, TX, USA.
    // All rights reserved.
    //
    char JetPort[20] = "";
    DWORD InQueue = 256, OutQueue = 256;
    long OpenError;
    DCB useDCB;
    int JetDrv = COM4;
    bool success;
    char s[80];
    unsigned char Input[80];
    int SizeInput = sizeof(Input) / sizeof(Input[0]);

    //	Check command line arguments.

    // Open COM port.
    sprintf(JetPort, "COM%d", JetDrv + 1);
    // Open communications.
    if (JetDrv != NOCOM) {
        hCom = CreateFile("\\\\.\\COM4", GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL); //USING COM4
        if (hCom == INVALID_HANDLE_VALUE) {
            LPVOID lpMsgBuf;
            OpenError = GetLastError();
            //str: fmtJetCommCallFailedWithError
            sprintf(s, fmtCommError, "CreateFile", OpenError);
            TestLog(s, true);
            if (FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, OpenError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR)&lpMsgBuf, 0, NULL) > 0) {
                strncpy(s, (const char*)lpMsgBuf, sizeof(s) - 1);
                s[sizeof(s) - 1] = '\0';
                TestLog(s, false);
                delete lpMsgBuf;
            }
            hCom = noCom;
            JetDrv = NOCOM;
            return 1;
        }
        SetupComm(hCom, InQueue, OutQueue);
        if (GetCommState(hCom, &useDCB)) {
            //str: fmtJetSizeOfHDCBIsBytes
            sprintf(s, "Size of DCB is %d bytes.\n", sizeof(useDCB));
            TestLog(s, false);
            useDCB.BaudRate = 9600;
            useDCB.ByteSize = 8;
            useDCB.Parity = NOPARITY;
            useDCB.StopBits = ONESTOPBIT;
            useDCB.fBinary = 0x1;
            //str: strJetCallingSetCommState
            TestLog("Calling SetCommState now.\n", false);
            if (!SetCommState(hCom, &useDCB)) {
                OpenError = GetLastError();
                //str: fmtJetCommCallFailedWithError
                sprintf(s, fmtCommError, "SetCommState", OpenError);
                TestLog(s, true);
                CloseHandle(hCom);
                hCom = noCom;
                JetDrv = NOCOM;
                return 1;
            }
        }
        else {
            OpenError = GetLastError();
            //str: fmtJetCommCallFailedWithError
            sprintf(s, fmtCommError, "GetCommState", OpenError);
            TestLog(s, true);
            CloseHandle(hCom);
            hCom = noCom;
            JetDrv = NOCOM;
            return 1;
        }




        // Start-up for MicroJet III.
        if (gDreamController && !gMultiChannel && !gBreadboardOne) {
            char QCmd[] = "Q", XCmd[] = "X2000";
            char Dummy[] = "Q - This is a dummy response. BEEP>";
            //              Q?Invalid command or format?      >
            int  l1 = strlen(Dummy), l2 = 49 - l1,
                lQ = strlen(QCmd), lX = strlen(XCmd);
            char WriteCmd[20];
            int iCmd;
            DWORD Length, OneLength, ReadLength, SentLength;
            DWORD DOne = (DWORD)1L;
            BOOL ReadDone, WriteDone;
            // Stop firmware program, in case it is running.
            Length = (DWORD)lQ;
            if (JetDrv != NOCOM)
                WriteDone = WriteFile(hCom, (LPCVOID)QCmd, Length,
                    &SentLength, NULL);
            else {
                WriteDone = TRUE;
                SentLength = Length;
            }
            TestLog(QCmd, false);
            if (WriteDone && SentLength == Length) {
                WaitSeconds(0.5);
                // Read response to Q command (35 or 49 bytes).
                if (JetDrv != NOCOM) {
                    Length = 0;
                    ReadDone = ReadFile(hCom, s, (DWORD)l1, &Length, NULL);
                    if (!ReadDone) Length = 0;
                    s[Length] = '\0';
                }
                else {
                    ReadDone = TRUE;
                    Length = (DWORD)l1;
                    strcpy(s, Dummy);
                }
                if (strstr(s, "MFJET32") != NULL) {
                    Length = (DWORD)(l1 + 2);
                    gVs6Kludge = true;
                    WaitSeconds(2.0);
                    PurgeComm(hCom, PURGE_RXCLEAR);
                }
                if (ReadDone && Length == (DWORD)l1) {
                    if (strncmp(s, QCmd, lQ) != 0) {		// Always false for Dummy.
                        ReadDone = ReadFile(hCom, &s[Length], (DWORD)l2,
                            &ReadLength, NULL);
                        if (ReadDone && ReadLength >= (DWORD)l2) {
                            Length += ReadLength;
                        }
                        WaitSeconds(3.0);	// Wait some after "Q" failed.
                    }
                    s[Length] = '\0';
                }
                if (ReadDone)
                    TestLog(s, false);
                else {
                    if (JetDrv != NOCOM) CloseHandle(hCom);
                    hCom = noCom;
                    JetDrv = NOCOM;
                    return 1;
                }
                // Start firmware program (again).
                Length = (DWORD)lX;
                if (JetDrv != NOCOM) {
                    SentLength = 0L;
                    ReadLength = 0L;
                    for (iCmd = 0; iCmd < lX; iCmd++) {
                        if (JetDrv != NOCOM)
                            WriteDone = WriteFile(hCom, &XCmd[iCmd], DOne,
                                &OneLength, NULL);
                        else {
                            WriteDone = TRUE;
                            OneLength = DOne;
                        }
                        if (!WriteDone || OneLength != DOne) {
                            SentLength = (DWORD)iCmd;
                            break;
                        }
                        SentLength = (DWORD)(iCmd + 1);
                        WaitSeconds(0.1);
                        if (JetDrv != NOCOM)
                            ReadDone = ReadFile(hCom, &s[iCmd], DOne,
                                &OneLength, NULL);
                        else {
                            ReadDone = TRUE;
                            OneLength = DOne;
                            s[iCmd] = XCmd[iCmd];
                        }
                        if (!ReadDone || OneLength != DOne) {
                            ReadLength = (DWORD)iCmd;
                            s[ReadLength] = '\0';
                            break;
                        }
                        ReadLength = (DWORD)(iCmd + 1);
                        s[ReadLength] = '\0';
                    }
                    if (ReadLength == (DWORD)lX) {
                        if (JetDrv != NOCOM && !gVs6Kludge)
                            ReadDone = ReadFile(hCom, &s[lX], (DWORD)2L,
                                &OneLength, NULL);
                        else {
                            ReadDone = TRUE;
                            strcpy(&s[lX], "\r\n");
                            OneLength = (DWORD)2L;
                        }
                        if (ReadDone) ReadLength += OneLength;
                        s[ReadLength] = '\0';
                    }
                }
                else {
                    SentLength = Length;
                    ReadLength = Length + (DWORD)2L;
                    sprintf(s, "%s\r\n", XCmd);
                }
                strcpy(WriteCmd, XCmd);
                WriteCmd[SentLength] = '\0';
                TestLog(WriteCmd, false);
                if (ReadLength != Length && SentLength == ReadLength) {
                    if (JetDrv != NOCOM) CloseHandle(hCom);
                    hCom = noCom;
                    JetDrv = NOCOM;
                    return 1;
                }
                if (SentLength == (DWORD)lX) {
                    // Wait a little for the start-up to complete.
                    WaitSeconds(1.0);
                }
                else {
                    // Start command did not get out the door, close down.
                    if (JetDrv != NOCOM) CloseHandle(hCom);
                    hCom = noCom;
                    JetDrv = NOCOM;
                    return 1;
                }
            }
            else {
                // Stop command did not get out the door, close down.
                if (JetDrv != NOCOM) CloseHandle(hCom);
                hCom = noCom;
                JetDrv = NOCOM;
                return 1;
            }
        }
    }
    // Initialize the pulser.
    // CUSTOM CODE
    // See command reference documentation for correct structure for sending commands to the JetDrive
    unsigned char jetcommands[128];
    unsigned char jetdriveinput[128];
    unsigned int commandlength = 0;
    unsigned int port = JetDrv + 1;


    SendCommand(port, MFJDRV_RESET, 3);         // 1.) Soft Reset
    SendCommand(port, MFJDRV_GETVERSION, .1);   // 2.) Version
    // 3.) Get Number of Channels (Multi-channel Only) don't need to do this
    SendCommand(port, MFJDRV_PULSE, .55);       // 4.) Pulse Wave Form
    SendCommand(port, MFJDRV_CONTMODE, .1);     // 5.) Trigger Mode
    SendCommand(port, MFJDRV_DROPS, 1);         // 6.) Number of Drops/Trigger
    SendCommand(port, MFJDRV_FULLFREQ, 1);      // 7.) Frequency
    SendCommand(port, MFJDRV_STROBEDIV, .1);    // 8.) Strobe Divider
    SendCommand(port, MFJDRV_STROBEENABLE, .1); // 9.) Strobe Enable
    SendCommand(port, MFJDRV_STROBEDELAY, .1);  // 10.) Strobe Delay
    SendCommand(port, MFJDRV_SOURCE, .1);       // 11.) Trigger Source


    // Start Jetting
    SendCommand(port, MFJDRV_SOFTTRIGGER, .1);
    gJets[gCJ]->fMode = 0;
    WaitSeconds(5);
    SendCommand(port, MFJDRV_CONTMODE, .1);

    // Loop for commands.
    for (;;) {

    }	// ... end of command processing loop.

    if (JetDrv != NOCOM && hCom != noCom) CloseHandle(hCom);
    return 0;
}


    