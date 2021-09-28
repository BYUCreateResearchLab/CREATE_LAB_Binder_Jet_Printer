#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "JetServer.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
#include <thread>


using namespace std;

void e(GReturn rc)
 {
   if (rc != G_NO_ERROR)
     throw rc;
 }

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

int jetter_setup() {
    char JetPort[20] = "" ;
        DWORD InQueue = 256, OutQueue = 256 ;
        long OpenError ;
        DCB useDCB ;
        int JetDrv = COM4 ;
        bool success ;
        char s[80] ;
        unsigned char Input[80] ;
        int SizeInput = sizeof(Input) / sizeof(Input[0]) ;

    //	Check command line arguments.

    // Open COM port.
    sprintf(JetPort, "COM%d", JetDrv+1) ;
    // Open communications.
    if (JetDrv != NOCOM) {
        hCom = CreateFile(L"\\\\.\\COM4", GENERIC_READ | GENERIC_WRITE,
                            0, NULL, OPEN_EXISTING, 0, NULL) ; //USING COM4
        if (hCom == INVALID_HANDLE_VALUE) {
            LPVOID lpMsgBuf;
            OpenError = GetLastError() ;
            //str: fmtJetCommCallFailedWithError
            sprintf(s, fmtCommError, "CreateFile", OpenError) ;
            TestLog(s, true) ;
            if (FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, OpenError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf, 0, NULL) > 0) {
                strncpy(s, (const char*)lpMsgBuf, sizeof(s)-1) ;
                s[sizeof(s)-1] = '\0' ;
                TestLog(s, false) ;
                delete lpMsgBuf ;
            }
            hCom = noCom ;
            JetDrv = NOCOM ;
            return 1 ;
        }
        SetupComm(hCom, InQueue, OutQueue) ;
        if (GetCommState(hCom, &useDCB)) {
        //str: fmtJetSizeOfHDCBIsBytes
            sprintf(s, "Size of DCB is %d bytes.\n", sizeof(useDCB)) ;
            TestLog(s, false) ;
            useDCB.BaudRate = 9600 ;
            useDCB.ByteSize = 8 ;
            useDCB.Parity = NOPARITY ;
            useDCB.StopBits = ONESTOPBIT ;
            useDCB.fBinary = 0x1 ;
            //str: strJetCallingSetCommState
            TestLog("Calling SetCommState now.\n", false) ;
            if (!SetCommState(hCom, &useDCB)) {
                OpenError = GetLastError() ;
            //str: fmtJetCommCallFailedWithError
                sprintf(s, fmtCommError, "SetCommState", OpenError) ;
                TestLog(s, true) ;
                CloseHandle(hCom) ;
            hCom = noCom ;
            JetDrv = NOCOM ;
            return 1 ;
            }
        }
        else {
            OpenError = GetLastError() ;
            //str: fmtJetCommCallFailedWithError
            sprintf(s, fmtCommError, "GetCommState", OpenError) ;
            TestLog(s, true) ;
            CloseHandle(hCom) ;
            hCom = noCom ;
            JetDrv = NOCOM ;
            return 1 ;
        }

        // Start-up for MicroJet III.
        if (gDreamController && !gMultiChannel && !gBreadboardOne) {
            char QCmd[] = "Q", XCmd[] = "X2000" ;
            char Dummy[] = "Q - This is a dummy response. BEEP>" ;
            //              Q?Invalid command or format?      >
            int  l1 = strlen(Dummy), l2 = 49 - l1,
                lQ = strlen(QCmd), lX = strlen(XCmd) ;
            char WriteCmd[20] ;
            int iCmd ;
            DWORD Length, OneLength, ReadLength, SentLength ;
            DWORD DOne = (DWORD)1L ;
            BOOL ReadDone, WriteDone ;
            // Stop firmware program, in case it is running.
            Length = (DWORD)lQ ;
            if (JetDrv != NOCOM)
                WriteDone = WriteFile(hCom, (LPCVOID)QCmd, Length,
                                        &SentLength, NULL) ;
            else {
            WriteDone = TRUE ;
            SentLength = Length ;
            }
            TestLog(QCmd, false) ;
            if (WriteDone && SentLength == Length) {
                WaitSeconds(0.5) ;
            // Read response to Q command (35 or 49 bytes).
            if (JetDrv != NOCOM) {
                Length = 0 ;
                    ReadDone = ReadFile(hCom, s, (DWORD)l1, &Length, NULL) ;
                if (!ReadDone) Length = 0 ;
                s[Length] = '\0' ;
            }
            else {
                ReadDone = TRUE ;
                Length = (DWORD)l1 ;
                strcpy(s, Dummy) ;
            }
            if (strstr(s, "MFJET32") != NULL) {
                Length = (DWORD)(l1 + 2) ;
                gVs6Kludge = true ;
                WaitSeconds(2.0) ;
                PurgeComm(hCom, PURGE_RXCLEAR) ;
            }
            if (ReadDone && Length == (DWORD)l1) {
                if (strncmp(s, QCmd, lQ) != 0) {		// Always false for Dummy.
                    ReadDone = ReadFile(hCom, &s[Length], (DWORD)l2,
                                                &ReadLength, NULL) ;
                    if (ReadDone && ReadLength >= (DWORD)l2) {
                        Length += ReadLength ;
                    }
                    WaitSeconds(3.0) ;	// Wait some after "Q" failed.
                }
                s[Length] = '\0' ;
            }
            if (ReadDone)
                TestLog(s, false) ;
            else {
                    if (JetDrv != NOCOM) CloseHandle(hCom) ;
                hCom = noCom ;
                JetDrv = NOCOM ;
                return 1 ;
            }
            // Start firmware program (again).
            Length = (DWORD)lX ;
            if (JetDrv != NOCOM) {
                SentLength = 0L ;
                ReadLength = 0L ;
                for (iCmd=0; iCmd<lX; iCmd++) {
                if (JetDrv != NOCOM)
                        WriteDone = WriteFile(hCom, &XCmd[iCmd], DOne,
                                                &OneLength, NULL) ;
                    else {
                    WriteDone = TRUE ;
                        OneLength = DOne ;
                    }
                    if (!WriteDone || OneLength != DOne) {
                    SentLength = (DWORD)iCmd ;
                        break ;
                    }
                    SentLength = (DWORD)(iCmd + 1) ;
                    WaitSeconds(0.1) ;
                    if (JetDrv != NOCOM)
                        ReadDone = ReadFile(hCom, &s[iCmd], DOne,
                                            &OneLength, NULL) ;
                    else {
                    ReadDone = TRUE ;
                        OneLength = DOne ;
                        s[iCmd] = XCmd[iCmd] ;
                    }
                    if (!ReadDone || OneLength != DOne) {
                    ReadLength = (DWORD)iCmd ;
                        s[ReadLength] = '\0' ;
                        break ;
                    }
                    ReadLength = (DWORD)(iCmd + 1) ;
                    s[ReadLength] = '\0' ;
                }
                if (ReadLength == (DWORD)lX) {
                if (JetDrv != NOCOM && !gVs6Kludge)
                        ReadDone = ReadFile(hCom, &s[lX], (DWORD)2L,
                                                &OneLength, NULL) ;
                    else {
                    ReadDone = TRUE ;
                    strcpy(&s[lX], "\r\n") ;
                        OneLength = (DWORD)2L ;
                    }
                    if (ReadDone) ReadLength += OneLength ;
                    s[ReadLength] = '\0' ;
                }
            }
            else {
                SentLength = Length ;
                ReadLength = Length + (DWORD)2L ;
                sprintf(s, "%s\r\n", XCmd) ;
            }
            strcpy(WriteCmd, XCmd) ;
            WriteCmd[SentLength] = '\0' ;
            TestLog(WriteCmd, false) ;
            if (ReadLength != Length && SentLength == ReadLength) {
                    if (JetDrv != NOCOM) CloseHandle(hCom) ;
                hCom = noCom ;
                JetDrv = NOCOM ;
                return 1 ;
            }
            if (SentLength == (DWORD)lX) {
                    // Wait a little for the start-up to complete.
                    WaitSeconds(1.0) ;
                }
                else {
                // Start command did not get out the door, close down.
                    if (JetDrv != NOCOM) CloseHandle(hCom) ;
                    hCom = noCom ;
                    JetDrv = NOCOM ;
                return 1 ;
            }
            }
            else {
            // Stop command did not get out the door, close down.
                if (JetDrv != NOCOM) CloseHandle(hCom) ;
                hCom = noCom ;
                JetDrv = NOCOM ;
            return 1 ;
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

       if (JetDrv != NOCOM && hCom != noCom) CloseHandle(hCom) ;
        return 0 ;
}

MainWindow::MainWindow(QMainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //Set up Second Window
    sWindow = new progWindow();
    connect(sWindow, &progWindow::firstWindow, this, &MainWindow::show);

    //Initialize Necessary Variables
    ui->bedSpinBox->setMaximum(300);
    delta_x = 10;
    delta_y = 10;
    delta_z = 15;
    //micronX = ;
    //micronY = ;
    micronZ = 7578;
    //mmX = ;
    //mmY = ;
    mmZ = 757760;
    //Update default GUI values
    MainWindow::on_revertDefault_clicked();

    //Disable all buttons that require a controller connection
    ui->activateHopper->setDisabled(true);
    ui->activateRoller1->setDisabled(true);
    ui->activateRoller2->setDisabled(true);
    ui->xHome->setDisabled(true);
    ui->yHome->setDisabled(true);
    ui->zHome->setDisabled(true);
    ui->xPositive->setDisabled(true);
    ui->yPositive->setDisabled(true);
    ui->xNegative->setDisabled(true);
    ui->yNegative->setDisabled(true);
    ui->zDown->setDisabled(true);
    ui->zUp->setDisabled(true);
    ui->zMax->setDisabled(true);
    ui->zMin->setDisabled(true);
    ui->spreadNewLayer->setDisabled(true);
}

MainWindow::~MainWindow()
{
    // On application close
    delete ui;

    if(g){ // if there is an active connection to a controller
        e(GCmd(g, "MO")); // Turn off the motors
        GClose(g);} // Close the connection to the controller

}


void MainWindow::on_yPositive_clicked()
{
    ui->yPositive->setText("Oh that tickled!");
    ui->xPositive->setText(">");
    ui->yNegative->setText("v");
    ui->xNegative->setText("<");

    int yVel = 1000*ui->yVelocity->value();
    int yDis = 1000*ui->yDistance->value();
    string yVelString = "SPY=" + to_string(yVel);
    string yDisString = "PRY=-" + to_string(yDis);
    if(g){
        e(GCmd(g, "ACY=50000")); // 50 mm/s^2
        e(GCmd(g, "DCY=50000")); // 50 mm/s^2
        e(GCmd(g, yVelString.c_str())); // 10 mm/s
        e(GCmd(g, yDisString.c_str()));  // -10 mm
        e(GCmd(g, "BGY"));
        e(GMotionComplete(g, "Y"));
    }
}

void MainWindow::on_xPositive_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText("Oh that tickled!");
    ui->yNegative->setText("v");
    ui->xNegative->setText("<");

    int xVel = 1000*ui->xVelocity->value();
    int xDis = 1000*ui->xDistance->value();
    string xVelString = "SPX=" + to_string(xVel);
    string xDisString = "PRX=" + to_string(xDis);
    if(g){
        e(GCmd(g, "ACX=50000")); // 50 mm/s^2
        e(GCmd(g, "DCX=50000")); // 50 mm/s^2
        e(GCmd(g, xVelString.c_str()));
        e(GCmd(g, xDisString.c_str()));
        e(GCmd(g, "BGX"));
        e(GMotionComplete(g, "X"));
    }

}


void MainWindow::on_yNegative_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText(">");
    ui->yNegative->setText("Oh that tickled!");
    ui->xNegative->setText("<");

    int yVel = 1000*ui->yVelocity->value();
    int yDis = 1000*ui->yDistance->value();
    string yVelString = "SPY=" + to_string(yVel);
    string yDisString = "PRY=" + to_string(yDis);
    if(g){
        e(GCmd(g, "ACY=50000")); // 50 mm/s^2
        e(GCmd(g, "DCY=50000")); // 50 mm/s^2
        e(GCmd(g, yVelString.c_str()));
        e(GCmd(g, yDisString.c_str()));
        e(GCmd(g, "BGY"));
        e(GMotionComplete(g, "Y"));
    }
}


void MainWindow::on_xNegative_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText(">");
    ui->yNegative->setText("v");
    ui->xNegative->setText("Oh that tickled!");

    int xVel = 1000*ui->xVelocity->value();
    int xDis = 1000*ui->xDistance->value();
    string xVelString = "SPX=" + to_string(xVel);
    string xDisString = "PRX=-" + to_string(xDis);
    if(g){
        e(GCmd(g, "ACX=50000")); // 50 mm/s^2
        e(GCmd(g, "DCX=50000")); // 50 mm/s^2
        e(GCmd(g, xVelString.c_str()));
        e(GCmd(g, xDisString.c_str()));
        e(GCmd(g, "BGX"));
        e(GMotionComplete(g, "X"));
    }

}


void MainWindow::on_xHome_clicked()
{
    //ui->label4Fun->setText("Homing In On X");
    if(g){ // If connected to controller
        // Home the X-Axis using the central home sensor index pulse
        e(GCmd(g, "ACX=200000"));   // 200 mm/s^2
        e(GCmd(g, "DCX=200000"));   // 200 mm/s^2
        e(GCmd(g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
        e(GCmd(g, "BGX"));          // Start motion towards rear limit sensor
        e(GMotionComplete(g, "X")); // Wait until limit is reached
        e(GCmd(g, "JGX=15000"));    // 15 mm/s jog towards home sensor
        e(GCmd(g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
        e(GCmd(g, "FIX"));          // Find index command for x axis
        e(GCmd(g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
        e(GMotionComplete(g, "X")); // Wait until X stage finishes moving
        e(GCmd(g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
    }

}


void MainWindow::on_yHome_clicked()
{
    ui->label4Fun->setText("Homing In On Y");
    //TODO - HOME Y AXIS ONCE THE LIMIT SENSORS ARE INSTALLED
    if(g){
        e(GCmd(g, "ACY=200000"));   // 200 mm/s^2
        e(GCmd(g, "DCY=200000"));   // 200 mm/s^2
        e(GCmd(g, "JGY=25000"));   // 15 mm/s jog towards rear limit
        e(GCmd(g, "BGY"));          // Start motion towards rear limit sensor
        e(GMotionComplete(g, "Y")); // Wait until limit is reached
        e(GCmd(g, "ACY=50000")); // 50 mm/s^2
        e(GCmd(g, "DCY=50000")); // 50 mm/s^2
        e(GCmd(g, "SPY=25000")); // 25 mm/s
        e(GCmd(g, "PRY=-200000"));  // 201.5 mm
        e(GCmd(g, "BGY"));
        e(GMotionComplete(g, "Y"));
    }
}

void MainWindow::on_zHome_clicked()
{
    ui->label4Fun->setText("Homing In On Z");
    if(g){ // If connected to controller
        // Home the Z-Axis using an offset from the top limit sensor
        e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(g, "JGZ=113664"));    // Speed of Z
        try {
            e(GCmd(g, "BGZ")); // Start motion towards rear limit sensor
        } catch(...) {}
        e(GMotionComplete(g, "Z")); // Wait until limit is reached
        e(GCmd(g, "ACZ=757760"));
        e(GCmd(g, "DCZ=757760"));
        e(GCmd(g, "SPZ=113664"));
        e(GCmd(g, "PRZ=-1000000"));//TODO - TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!
        e(GCmd(g, "BGZ"));
        e(GMotionComplete(g, "Z")); // Wait until limit is reached
        e(GCmd(g, "DPZ=0"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
    }
}


void MainWindow::on_zStepSize_valueChanged(int arg1)
{
    delta_z = arg1;
}


void MainWindow::on_zMax_clicked()
{
    z_position = 300;
    ui->bedSpinBox->setValue(z_position);

    e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
    e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
    e(GCmd(g, "JGZ=113664"));    // Speed of Z
    try {
        e(GCmd(g, "BGZ")); // Start motion towards rear limit sensor
    } catch(...) {}
    e(GMotionComplete(g, "Z")); // Wait until limit is reached
}

void  MainWindow::on_zUp_clicked()
{

    if(g){
        z_position = z_position + delta_z;
        ui->bedSpinBox->setValue(z_position);
        //DMC Commands : https://www.galil.com/download/comref/com4000/index.html#cover.html+DMC4000

        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=" + to_string(zSteps);
        e(GCmd(g, "ACZ=757760"));         //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(g, "DCZ=757760"));         //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(g, "SPZ=113664"));         //Speed of C
        e(GCmd(g, zPRZString.c_str()));   //Position Relative of C //HOW TO SWITCH THIS TO GCSTRING IN?
        try {
            e(GCmd(g, "BGZ"));            //Begin Motion
        } catch(...) {}
        e(GMotionComplete(g, "Z"));       //Waits until motion is complete?
    }

}

void  MainWindow::on_zDown_clicked()
{

    if(g){
        z_position = z_position - delta_z;
        ui->bedSpinBox->setValue(z_position);

        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=-" + to_string(zSteps);
        e(GCmd(g, "ACZ=757760"));
        e(GCmd(g, "DCZ=757760"));
        e(GCmd(g, "SPZ=113664"));
        e(GCmd(g,  zPRZString.c_str()));
        try {
            e(GCmd(g, "BGZ")); // Begin motion
        } catch(...) {}
        e(GMotionComplete(g, "Z"));
    }
}

void  MainWindow::on_zMin_clicked()
{
    z_position = 0;
    ui->bedSpinBox->setValue(z_position);

    e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
    e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
    e(GCmd(g, "JGZ=-113664"));    // Speed of Z
    try {
        e(GCmd(g, "BGZ")); // Start motion towards rear limit sensor
    } catch(...) {}
    e(GMotionComplete(g, "Z")); // Wait until limit is reached
}



void MainWindow::on_activateRoller1_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateRoller1->setText("Roller Activated!");
    }
    else {
        ui->activateRoller1->setText("Activate Roller");
    }
}

void MainWindow::on_activateRoller2_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateRoller2->setText("Roller Activated!");
    }
    else {
        ui->activateRoller2->setText("Activate Roller");
    }
}


void MainWindow::on_activateHopper_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateHopper->setText("Hopper Activated!");
    }
    else {
        ui->activateHopper->setText("Activate Hopper");
    }
}


void MainWindow::on_connect_clicked()
{

    if(g==0){
        //TO DO - DISABLE BUTTONS UNTIL TUNED, SEPERATE INITIALIZATION HOMING BUTTONS

         //TODO Maybe Threading or something like that? The gui is unresponsive until connect function is finished.
         e(GOpen(address, &g)); // Establish connection with motion controller
         e(GCmd(g, "SH XYZ")); // Enable X,Y, and Z motors

         // Controller Configuration
         e(GCmd(g, "MO")); // Ensure motors are off for setup

         // X Axis
         e(GCmd(g, "MTX = -1"));    // Set motor type to reversed brushless
         e(GCmd(g, "CEX = 2"));     // Set Encoder to reversed quadrature
         e(GCmd(g, "BMX = 40000")); // Set magnetic pitch of lienar motor
         e(GCmd(g, "AGX = 1"));     // Set amplifier gain
         e(GCmd(g, "AUX = 9"));     // Set current loop (based on inductance of motor)
         e(GCmd(g, "TLX = 3"));     // Set constant torque limit to 3V
         e(GCmd(g, "TKX = 0"));     // Disable peak torque setting for now

         // Y Axis
         e(GCmd(g, "MTY = 1"));     // Set motor type to standard brushless
         e(GCmd(g, "CEY = 0"));     // Set Encoder to reversed quadrature
         e(GCmd(g, "BMY = 2000"));  // Set magnetic pitch of rotary motor
         e(GCmd(g, "AGY = 1"));     // Set amplifier gain
         e(GCmd(g, "AUY = 11"));    // Set current loop (based on inductance of motor)
         e(GCmd(g, "TLY = 6"));     // Set constant torque limit to 6V
         e(GCmd(g, "TKY = 0"));     // Disable peak torque setting for now

         // Z Axis
         e(GCmd(g, "MTZ = -2.5"));  // Set motor type to standard brushless
         e(GCmd(g, "CEZ = 14"));    // Set Encoder to reversed quadrature
         e(GCmd(g, "AGZ = 0"));     // Set amplifier gain
         e(GCmd(g, "AUZ = 9"));     // Set current loop (based on inductance of motor)
         // Note: There might be more settings especially for this axis I might want to add later

         // H Axis (Jetting Axis)
         e(GCmd(g, "MTH = -2"));    // Set jetting axis to be stepper motor with defualt low
         e(GCmd(g, "AGH = 0"));     // Set gain to lowest value
         e(GCmd(g, "LDH = 3"));     // Disable limit sensors for H axis
         e(GCmd(g, "KSH = .25"));   // Minimize filters on step signals
         e(GCmd(g, "ITH = 1"));     // Minimize filters on step signals

         e(GCmd(g, "BN"));          // Save (burn) these settings to the controller just to be safe

         e(GCmd(g, "SH XYZ"));      // Enable X,Y, and Z motors
         e(GCmd(g, "CN= -1"));      // Set correct polarity for all limit switches

         //HOME ALL AXIS'
         if(g){
             // Home the X-Axis using the central home sensor index pulse
             e(GCmd(g, "ACX=200000"));   // 200 mm/s^2
             e(GCmd(g, "DCX=200000"));   // 200 mm/s^2
             e(GCmd(g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
             e(GCmd(g, "ACY=200000"));   // 200 mm/s^2
             e(GCmd(g, "DCY=200000"));   // 200 mm/s^2
             e(GCmd(g, "JGY=25000"));   // 15 mm/s jog towards rear limit
             e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
             e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
             e(GCmd(g, "JGZ=113664"));    // Speed of Z
             try {
                e(GCmd(g, "BGX"));          // Start motion towards rear limit sensor
                e(GCmd(g, "BGY"));          // Start motion towards rear limit sensor
                e(GCmd(g, "BGZ")); // Start motion towards rear limit sensor
                e(GMotionComplete(g, "X")); // Wait until limit is reached
                e(GMotionComplete(g, "Y")); // Wait until limit is reached
                e(GMotionComplete(g, "Z")); // Wait until limit is reached
             } catch(...) {}
             e(GCmd(g, "JGX=15000"));    // 15 mm/s jog towards home sensor
             e(GCmd(g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
             e(GCmd(g, "FIX"));          // Find index command for x axis
             e(GCmd(g, "ACY=50000")); // 50 mm/s^2
             e(GCmd(g, "DCY=50000")); // 50 mm/s^2
             e(GCmd(g, "SPY=25000")); // 25 mm/s
             e(GCmd(g, "PRY=-200000"));  // 201.5 mm
             e(GCmd(g, "ACZ=757760"));
             e(GCmd(g, "DCZ=757760"));
             e(GCmd(g, "SDZ=1515520")); // Sets deceleration when limit switch is touched
             e(GCmd(g, "SPZ=113664"));
             e(GCmd(g, "PRZ=-100000"));//TODO - TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!
             e(GCmd(g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
             e(GCmd(g, "BGY"));
             e(GCmd(g, "BGZ"));
             e(GMotionComplete(g, "X")); // Wait until X stage finishes moving
             e(GMotionComplete(g, "Y"));
             e(GMotionComplete(g, "Z")); // Wait until limit is reached
             e(GCmd(g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
             e(GCmd(g, "DPY=0"));
             e(GCmd(g, "DPZ=0"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)

     ui->connect->setText("Disconnect Controller");

     ui->activateHopper->setDisabled(false);
     ui->activateRoller1->setDisabled(false);
     ui->activateRoller2->setDisabled(false);
     ui->xHome->setDisabled(false);
     ui->yHome->setDisabled(false);
     ui->zHome->setDisabled(false);
     ui->xPositive->setDisabled(false);
     ui->yPositive->setDisabled(false);
     ui->xNegative->setDisabled(false);
     ui->yNegative->setDisabled(false);
     ui->zDown->setDisabled(false);
     ui->zUp->setDisabled(false);
     ui->zMax->setDisabled(false);
     ui->zMin->setDisabled(false);
     ui->spreadNewLayer->setDisabled(false);
        }
    }
    else{
        e(GCmd(g, "MO"));       // Disable Motors
        GClose(g);
        g = 0;                  // Reset connection handle
        ui->connect->setText("Connect to Controller");

        ui->activateHopper->setDisabled(true);
        ui->activateRoller1->setDisabled(true);
        ui->activateRoller2->setDisabled(true);
        ui->xHome->setDisabled(true);
        ui->yHome->setDisabled(true);
        ui->zHome->setDisabled(true);
        ui->xPositive->setDisabled(true);
        ui->yPositive->setDisabled(true);
        ui->xNegative->setDisabled(true);
        ui->yNegative->setDisabled(true);
        ui->zDown->setDisabled(true);
        ui->zUp->setDisabled(true);
        ui->zMax->setDisabled(true);
        ui->zMin->setDisabled(true);
        ui->spreadNewLayer->setDisabled(true);
    }
}


void MainWindow::on_OpenProgramWindow_clicked()
{
    sWindow->show();
    this->close();
}

void MainWindow::on_saveDefault_clicked()
{
    std::ofstream ofs;
    ofs.open("C:/Users/ME/Documents/GitHub/CREATE_LAB_Binder_Jet_Printer/PrinterSettings.txt", std::ofstream::out | std::ofstream::trunc);
    ofs<< "XAxisVelocity\t" << to_string(ui->xVelocity->value()) << "\n";
    ofs<< "YAxisVelocity\t" << to_string(ui->yVelocity->value()) << "\n";
    ofs<< "XAxisDistance\t" << to_string(ui->xDistance->value()) << "\n";
    ofs<< "YAxisDistance\t" << to_string(ui->yDistance->value()) << "\n";
    ofs<< "ZStepSize\t" << to_string(ui->zStepSize->value()) << "\n";
    ofs<< "RollerSpeed\t" << to_string(ui->rollerSpeed->value()) << "\n";
    ofs<< "NumberOfLayers\t" << to_string(ui->numLayers->value()) << "\n";
    ofs.close();
}

void MainWindow::on_revertDefault_clicked()
{
    string line;
    ifstream myfile("C:/Users/ME/Documents/GitHub/CREATE_LAB_Binder_Jet_Printer/PrinterSettings.txt");
    if (myfile.is_open())
    {
        while(getline(myfile, line)) {
            vector<string> row_values;

            split(line, '\t', row_values);

            if(row_values[0] == "XAxisVelocity")
            {
                ui->xVelocity->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "YAxisVelocity")
            {
                ui->yVelocity->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "XAxisDistance")
            {
                ui->xDistance->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "YAxisDistance")
            {
                ui->yDistance->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "ZStepSize")
            {
                ui->zStepSize->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "RollerSpeed")
            {
                ui->rollerSpeed->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "NumberOfLayers")
            {
                ui->numLayers->setValue(stoi(row_values[1]));
            }
        }
        myfile.close();
      }
    else cout << "Unable to open file";//Notify user-> "Unable to open file";
}

void MainWindow::on_spreadNewLayer_clicked()
{
    //Spread Layers
    if(g){
        for(int i = 0; i < ui->numLayers->value(); ++i) {
            e(GCmd(g, "PRY=-230000")); //tune starting point
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "Y"));

            e(GCmd(g, "SB 18"));    // Turns on rollers
            e(GCmd(g, "SB 21"));

            e(GCmd(g, "PRY=-250000")); //tune starting point
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "Y"));

            e(GCmd(g, "CB 18"));    // Turns on rollers
            e(GCmd(g, "CB 21"));
        }
    }
}


void MainWindow::on_activateRoller1_toggled(bool checked)
{
    if(checked == 1) {
        if(g){
            e(GCmd(g, "SB 18"));
        }
    } else {
        if(g){
            e(GCmd(g, "CB 18"));
        }
    }
}

void MainWindow::on_activateRoller2_toggled(bool checked)
{
    if(checked == 1) {
        if(g){
            e(GCmd(g, "SB 21"));
        }
    } else {
        if(g){
            e(GCmd(g, "CB 21"));
        }
    }
}


void MainWindow::on_activateJet_stateChanged(int arg1)
{
    jetter_setup();
}

