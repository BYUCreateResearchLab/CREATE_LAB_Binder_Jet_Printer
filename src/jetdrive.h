#ifndef JETDRIVE_H
#define JETDRIVE_H

typedef void* HANDLE;
class MicroJet;

#define MFJDRV_COM1 0
#define MFJDRV_COM2 1
#define MFJDRV_COM4 3
#define MFJDRV_COM9 8

class JetDrive
{
public:
    explicit JetDrive();
    ~JetDrive();

    int initialize_jet_drive();
    void set_continuous_jetting();
    void set_single_jetting();
    void set_continuous_mode_frequency(long frequency_Hz);
    void set_external_trigger();
    void set_internal_trigger();
    void set_echo_and_dwell_voltage(short echoVoltage_Volts, short dwellVoltage_Volts);
    void start_continuous_jetting();
    void stop_continuous_jetting();
    void enable_strobe();
    void disable_strobe();
    void set_strobe_delay(short strobeDelay_microseconds);

private:
    void wait_seconds(float seconds);
    void test_log(const char* outputstring, bool status);

    int build_command(int command, unsigned char *jetCommand);
    int get_from_jet_drive(int port, int command, unsigned char *input, int sizeInput);
    void send_to_jet_drive(int port, int command, unsigned char *output, int lengthOutput);
    void send_command(int port, int command, float waitTimeSeconds);

private:
    HANDLE hCom;
    int mJetDrv{MFJDRV_COM9};
    MicroJet *mJetSettings{nullptr};

    float defaultWaitTime{0.0001f};

    // don't change these
    HANDLE noCom;
    static const char fmtCommError[];
    static const int NOCOM;
    long gCheckLong{0L};
    int gFirmVersion{0};
    short gExternEnable{0};
    bool gVs6Kludge{false};
    double gRateLimit{30.0};
};

class MicroJet // Data structure for microjet operating and status parameters.
{
public:
    MicroJet();
    ~MicroJet();
    double fTRise;
    double fTDwell;
    double fTFall;
    double fTEcho;
    double fTFinal;
    double fTDelay;

    long fFrequency;

    short fUIdle;
    short fUDwell;
    short fUEcho;
    short fUGain;
    short fMode;
    short fSource;
    short fDrops;
    short fStrobeDelay;
    short fStrobeDiv;
    short fStrobeEnable;
    short fDebugSwitch;
    short fDebugValue;
    short fChannelGroup;
    short fContIsStarted;
    short fChannelOn;

private:
    static short fgNJets;
};

#endif // JETDRIVE_H
