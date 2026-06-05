#include "printer.h"

#include <sstream>
#include <cmath>
#include <stdexcept>

#include "pcd.h"
#include <gclib.h>
#include "jetdrive.h"
#include "dmc4080.h"
#include "mister.h"
#include "bedmicroscope.h"
#include "mjdriver.h"
#include "heatlamp.h"
#include "mjprintheadwidget.h"

Printer::Printer(QObject *parent) :
    QObject(parent),
    mcu ( new DMC4080("192.168.42.100", this) ),    // changed this from 192.168.42.100
    jetDrive ( new JetDrive::Controller("COM8", this) ),
    pressureController ( new PCD::Controller("COM3", this) ),
    mister ( new Mister::Controller("COM6", this) ),
    bedMicroscope ( new BedMicroscope(this) ),
    mjController ( new Added_Scientific::Controller("COM5", this) ),
    heatLamp ( new HeatLamp(40, this) )
{
//    using Added_Scientific::Controller::HeadIndex;------------------------------------------------
//    mjController->set_head_voltage(HeadIndex::HEAD1, 25);
    mjController->mode_select(Added_Scientific::Controller::Mode::MOTION_ENCODER);
}

Printer::~Printer()
{
    disconnect_printer();
}

void Printer::connect(bool homeZAxis)
{
    mcu->connect_to_motion_controller(homeZAxis);

    // connect to serial devices
    jetDrive->connect_to_jet_drive();
    //pressureController->connect_to_pressure_controller();
    //mister->connect_to_misters();
}

void Printer::disconnect_printer()
{
    mcu->disconnect_controller();

    jetDrive->disconnect_serial();
    pressureController->disconnect_serial();
    mister->disconnect_serial();
}


using namespace CMD::detail;

std::string CMD::detail::axis_string(Axis axis)
{
    switch (axis)
    {
    case Axis::X:   return {"X"};
    case Axis::Y:   return {"Y"};
    case Axis::Z:   return {"Z"};
    case Axis::Jet: return {"H"};
    case Axis::Reservoir: return {'E'};

    default:
        throw std::invalid_argument("invalid axis");
    }
}

std::string CMD::detail::int_to_axis_string(int analoginput)
{
    switch (analoginput)
    {
    case 1:     return {"X"};
    case 2:     return {"Y"};
    case 3:     return {"Z"};
    case 4:     return {"W"};
    case 5:     return {'E'};
    case 6:     return {"F"};
    case 7:     return {"G"};
    case 8:     return {'H'};

    default:
        throw std::invalid_argument("invalid axis");
    }
}

constexpr int CMD::detail::mm2cnts(double mm, Axis axis)
{
    switch (axis)
    {
    case Axis::X:   return (int)(mm * X_CNTS_PER_MM);
    case Axis::Y:   return (int)(mm * Y_CNTS_PER_MM);
    case Axis::Z:   return (int)(mm * Z_CNTS_PER_MM);
    case Axis::Jet: return (int)(mm);
    case Axis::Reservoir: return (int)(mm * R_CNTS_PER_MM); // MAX 03/04 !!! converts mm distance to steps

    default: throw std::invalid_argument("invalid axis");
    }
}

float Printer::motor_type_value(MotorType motorType)
{
    switch (motorType)
    {
    case MotorType::Servo: return 1;
    case MotorType::Servo_R: return -1;
    case MotorType::StepLow: return 2;
    case MotorType::StepLow_R: return 2.5;
    case MotorType::StepHigh: return -2;
    case MotorType::StepHigh_R: return -2.5;
    case MotorType::Servo2PB: return 4;
    case MotorType::Servo2PB_R: return -4;
    default: throw std::invalid_argument("invalid motor type");
    }
}

std::string CMD::set_default_controller_settings()
{
    using CMD::detail::GCmd;

    std::stringstream s;

    // Controller Configuration
    s << GCmd("MO")          // Ensure motors are off for setup

         // Controller Time Update Setting
      << GCmd("TM 500")      // Set the update time of the motion controller

         // X Axis
      << GCmd("MTX=-1")      // Set motor type to reversed brushless
      << GCmd("BAX")         // Set motor to brushless
      << GCmd("CEX=10")      // Set main and aux encoder to reversed quadrature
      << GCmd("BMX=40000")   // Set magnetic pitch of linear motor
      << GCmd("AGX=1")       // Set amplifier gain
      << GCmd("AUX=9")       // Set current loop (based on inductance of motor)
      << GCmd("TLX=3")       // Set constant torque limit to 3V
      << GCmd("TKX=0")       // Disable peak torque setting for now
         // Set PID Settings
         // (NOTE: PID SETTINGS ARE OPTIMIZED FOR TM 500.
         // NEED TO USE OTHER VALUES FOR TM 1000!)
      << GCmd("KDX=1000")    // Set Derivative
      << GCmd("KPX=100")     // Set Proportional
      << GCmd("KIX=0.5")     // Set Integral
      << GCmd("PLX=177")     // Set low-pass filter

         // Y Axis
      << GCmd("MTY=1")       // Set motor type to standard brushless
      << GCmd("CEY=0")       // Set encoder to normal quadrature
      << GCmd("BMY=2000")    // Set magnetic pitch of rotary motor
      << GCmd("AGY=1")       // Set amplifier gain
      << GCmd("AUY=11")      // Set current loop (based on inductance of motor)
      << GCmd("TLY=6")       // Set constant torque limit to 6V
      << GCmd("TKY=0")       // Disable peak torque setting for now
         // Set PID Settings
      << GCmd("KDY=2000")    // Set Derivative
      << GCmd("KPY=100")     // Set Proportional
      << GCmd("KIY=1")       // Set Integral
      << GCmd("PLY=50")      // Set low-pass filter

         // Z Axis
      << GCmd("MTZ=-2.5")    // Stepper motor with active high step pulses, reversed direction
      << GCmd("LDZ=1")       // Set only the reverse limit switch to work
      << GCmd("CEZ=14")      // Set encoder to reversed quadrature
      << GCmd("AGZ=0")       // Set amplifier gain
      << GCmd("AUZ=9")       // Set current loop (based on inductance of motor)
         // Note: There might be more settings especially for this axis I might want to add later

         // H Axis (Jetting Axis)
      << GCmd("MTH=-2")      // Set jetting axis to be stepper motor with defualt low
      << GCmd("AGH=0")       // Set gain to lowest value (to disable amplifier on external drivers)
      << GCmd("LDH=3")       // Disable limit sensors for H axis
      << GCmd("KSH=0.5")     // Minimize filters on step signals (0.25 when TM=1000)
      << GCmd("ITH=1" )      // Minimize filters on step signals
      << GCmd("YAH=1")       // set step resolution to 1 full step per step

      // MAX 03/04 !!!
       // Reservoir Axis (E)
      << GCmd("MTE=-2")      // Stepper motor with active high step pulses, reversed direction
      << GCmd("YAE=" + std::to_string(MICROSTEPPING)) // Step Resoluton
      << GCmd("AGE=1")       // Set amplifier gain
      << GCmd("AUE=2")       // Set current loop (based on inductance of motor)
      << GCmd("ALE=0")       // 1 = Active High, 0 = Active Low
      << GCmd("LDE=2")       // 2 = forward limit only, 3 = diable both limits
      << GCmd("FLE=2147483647")   // Disable Forward Software Limit (PREVENTS CRASHES)
      << GCmd("BLE=-2147483648")  // Disable Reverse Software Limit (PREVENTS CRASHES)
      << GCmd("DPE=0")            //Define current position as 0
      //<< GCmd("CNE=-1")         // Set polarity (use -1 for Normally Closed, 1 for Normally Open)

      // Pyrometer data array
      << GCmd("DM BEDTEMP[1]")
      << GCmd("DM BEDTEMPS[1000]");

    for(int i = 0; i < 1000; i++){ //clear BEDTEMPS array
        s << GCmd("BEDTEMPS[" + std::to_string(i) + "] = 0");
    }


         // Configure Extended I/O
      s << GCmd("CO 3")        // configures bank 2 and 3 as outputs on extended I/O (IO 17-32)
      << GCmd("SB " + std::to_string(HEATLAMP_D0))       // turn off heat lamp
      << GCmd("SB " + std::to_string(HEATLAMP_D1))
      << GCmd("SB " + std::to_string(HEATLAMP_D2))
      << GCmd("SB " + std::to_string(HEATLAMP_D3))

      << GCmd("CC 19200,0,1,0")  // AUX PORT FOR THE ULTRASONIC GENERATOR
      << GCmd("CN=-1, -1")           // Set correct polarity for all limit switches
      << GCmd("BN")              // Save (burn) these settings to the controller just to be safe
      << GCmd("SH XYZ")          // Enable X,Y, and Z motors
      << GCmd("SH H")            // Servo the jetting axis
      << GCmd("SH E");           // Servo the Reservior Axis !!!
    return s.str();
}


std::string CMD::add_pvt_data_to_buffer(
        Axis axis,
        double relativePosition_mm,
        double velocity_mm,
        int time_counts)
{
    std::string result;
    result += GCmd();
    result += "PV";
    result += axis_string(axis);
    result += "=";
    result += std::to_string(mm2cnts(relativePosition_mm, axis));
    result += ",";
    result += std::to_string(mm2cnts(velocity_mm, axis));
    result += ",";
    result += std::to_string(time_counts);
    result += "\n";
    return result;
}

std::string CMD::exit_pvt_mode(Axis axis)
{
    std::string result;
    result += GCmd();
    result += "PV";
    result += axis_string(axis);
    result += "=,,0";
    result += "\n";
    return result;
}

std::string CMD::begin_pvt_motion(Axis axis)
{
    return {GCmd() + "BT" + detail::axis_string(axis) + "\n"};
}

std::string CMD::at_time_samples(int samples)
{
    std::string result;
    result += GCmd();
    result += "AT ";
    result += std::to_string(samples);
    result += ",1";
    result += "\n";
    return result;
}

std::string CMD::at_time_milliseconds(int milliseconds)
{
    std::string result;
    result += GCmd();
    result += "AT ";
    result += std::to_string(milliseconds);
    result += "\n";
    return result;
}

std::string CMD::after_absolute_position(Axis axis, double absolutePosition_mm)
{
    return {create_gcmd("AP", axis, mm2cnts(absolutePosition_mm, axis))};
}

std::string CMD::set_hopper_mode_and_intensity(int mode, int intensity)
{
    // modes A-H (int mode is index 0-7)
    // intensity 100%-30% (int intensity is index 0-7)
    // "MG{P2} {^77}, {^48}, {^53}, {^13}{N}"
    // is the correct command for "M05" or Mode: 'A' and Intensity: 50%
    return GCmd() + "MG{P2} "
            + to_ASCII_code('M')
            + to_ASCII_code('0' + mode)
            + to_ASCII_code('0' + intensity)
            + "{^13}{N}" + "\n";
}

std::string CMD::move_xy_axes_to_default_position()
{
    std::stringstream s;
    s << CMD::set_speed(Axis::X, 60);
    s << CMD::set_jog(Axis::Y, 40);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    return s.str();
}

std::string CMD::mist_layer(double traverseSpeed_mm_per_s, int sleepTime_ms)
{
    std::stringstream s;

    const int yAxisTravelSpeed_mm_per_s = 60;
    const double startPosition_mm = -350;
    const double endPosition_mm = -150;
    const double zAxisOffsetUnderRoller{0.5};

    // setup
    s << message("Misting layer");
    //s << message("CMD MIST_OFF"); // just make sure that the mister is off
    s << set_accleration(Axis::Y, 600);
    s << set_deceleration(Axis::Y, 600);
    s << set_speed(Axis::Y, yAxisTravelSpeed_mm_per_s);

    s << set_accleration(Axis::Z, 10);
    s << set_deceleration(Axis::Z, 10);
    s << set_speed(Axis::Z, 2);
    s << position_relative(Axis::Z, -zAxisOffsetUnderRoller);
    s << begin_motion(Axis::Z);
    s << after_motion(Axis::Z);

    // move y-axis to start misting position
    s << position_absolute(Axis::Y, startPosition_mm);
    s << begin_motion(Axis::Y);
    s << after_motion(Axis::Y);

    s << message("CMD MIST_ON");
    s << wait(sleepTime_ms); // wait

    s << set_speed(Axis::Y, traverseSpeed_mm_per_s); // set traverse speed
    // travel under mister and specified speed
    s << position_absolute(Axis::Y, endPosition_mm);
    s << begin_motion(Axis::Y);
    s << after_motion(Axis::Y);

    s << message("CMD MIST_OFF");


    // move z-axis back up
    s << position_relative(Axis::Z, zAxisOffsetUnderRoller);
    s << begin_motion(Axis::Z);

    // move forward
    s << set_speed(Axis::Y, yAxisTravelSpeed_mm_per_s);
    s << position_absolute(Axis::Y, -50);
    s << begin_motion(Axis::Y);
    s << after_motion(Axis::Y);
    s << after_motion(Axis::Z);
    s << message("Misting complete");

    return s.str();
}

std::string CMD::set_jetting_gearing_ratio_from_droplet_spacing(
        Axis masterAxis,
        int dropletSpacing_um)
{
    double gearingRatio = (
                1000.0
                / ((double)dropletSpacing_um * mm2cnts(1, masterAxis)));
    return {GCmd()
                + "GR"
                + detail::axis_string(Axis::Jet)
                + "="
                + std::to_string(gearingRatio)
                + "\n"};
}

std::string CMD::homing_sequence(bool homeZAxis)
{
    std::stringstream s;

    // === Home the X-Axis using the central home sensor index pulse ===

    s << set_accleration(Axis::X, 800);
    s << set_deceleration(Axis::X, 800);
    s << set_limit_switch_deceleration(Axis::X, 800);
    s << set_jog(Axis::X, 25); // jog towards front limit

    s << set_accleration(Axis::Y, 400);
    s << set_deceleration(Axis::Y, 400);
    s << set_limit_switch_deceleration(Axis::Y, 600);
    s << set_jog(Axis::Y, 25); // jog towards front limit

    if (homeZAxis)
    {
        s << set_accleration(Axis::Z, 20);
        s << set_deceleration(Axis::Z, 20);
        s << set_limit_switch_deceleration(Axis::Z, 40);
        // jog to bottom (MAX SPEED of 5mm/s!)
        s << set_jog(Axis::Z, -2);
        // turn off top software limit
        //s << disable_forward_software_limit(Axis::Z);
    }

    // --- E-Axis Homing Additions ---
    // Assuming a moderate speed for the reservoir stepper
    s << set_accleration(Axis::Reservoir, 100);
    s << set_deceleration(Axis::Reservoir, 100);
    s << set_limit_switch_deceleration(Axis::Reservoir, 400);
    s << set_jog(Axis::Reservoir, 10); // Jog forward until limit switch hit
    s << disable_reverse_software_limit(Axis::Reservoir);
    // -------------------------------

    s << begin_motion(Axis::X);
    s << begin_motion(Axis::Y);
    if (homeZAxis)
        s << begin_motion(Axis::Z);
    s << begin_motion(Axis::Reservoir); //

    s << motion_complete(Axis::X);
    s << motion_complete(Axis::Y);

    // TODO: This is a temporary solution that depends on alignment
    // of ballscrew and motor index pulse
    // find a better way to do this
    // move y-axis forward a bit to avoid being right on top of the index pulse
    s << position_relative(Axis::Y, -5);
    s << set_speed(Axis::Y, 10);
    s << begin_motion(Axis::Y);
    s << motion_complete(Axis::Y);
    // =================================

    // this is put after the short y move because the z axis is slow
    if (homeZAxis)
        s << motion_complete(Axis::Z);

    // Wait for E-Axis to hit the forward limit
    s << motion_complete(Axis::Reservoir);

    s << sleep(1000);

    // home to center index on x axis
    s << set_jog(Axis::X, -30);
    s << set_homing_velocity(Axis::X, 0.5);
    s << find_index(Axis::X);

    // home y axis to nearest index
    s << set_jog(Axis::Y, -0.5);
    s << set_homing_velocity(Axis::Y, 0.25);
    s << find_index(Axis::Y);

    if (homeZAxis)
    {
        // slower acceleration for going back up
        s << set_accleration(Axis::Z, 10);
        s << set_speed(Axis::Z, 2);
        // TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!
        s << position_relative(Axis::Z, 13.5322);
    }


    s << begin_motion(Axis::X);
    s << begin_motion(Axis::Y);
    if (homeZAxis)
        s << begin_motion(Axis::Z);

    s << motion_complete(Axis::X);
    s << motion_complete(Axis::Y);
    if (homeZAxis)
        s << motion_complete(Axis::Z);
    s << define_position(Axis::X, X_STAGE_LEN_MM / 2.0);
    s << define_position(Axis::Y, 0);
    s << define_position(Axis::Z, 0);
    s << define_position(Axis::Reservoir, 0); // Set E forward limit position as 0

    // set software limit to current position
    //s << set_forward_software_limit(Axis::Z, 500);
    if (homeZAxis){
        double zSoftwareLimit_mm = 15.0 - 13.5322;
        s << set_forward_software_limit(Axis::Z, zSoftwareLimit_mm);
    }

    // Set reverse software limit for E (prevents it from crashing backward)
    // Replace REVOIR_TRAVEL_LIMIT with your actual travel distance (e.g., -50)
    s << set_reverse_software_limit(Axis::Reservoir, -100.0);

/*
    // === Home Reservoir Axis to Single Limit Switch === (added 3/11)
    s << set_accleration(Axis::Reservoir, 200);
    s << set_deceleration(Axis::Reservoir, 200);
   // s << set_limit_switch_deceleration(Axis::Reservoir, 400);

    // Jog towards the physical limit switch.
    s << set_jog(Axis::Reservoir, 5*R_STEP_RESOLUTION); // jog into upper limit
    s << begin_motion(Axis::Reservoir); // Start Reservoir homing
    s << motion_complete(Axis::Reservoir); // Wait for Reservoir to hit the physical limit
    // Perform the Back-off (Negative = Down)
    s << position_relative(Axis::Reservoir, -3);
    s << set_speed(Axis::Reservoir, 10); //Sets the speed for the relative back-off move
    s << begin_motion(Axis::Reservoir);
    s << motion_complete(Axis::Reservoir);

    // Define final Reservoir position and software limits
    s << define_position(Axis::Reservoir, R_STAGE_LEN_MM);
    s << set_forward_software_limit(Axis::Reservoir, R_STAGE_LEN_MM); // Can't go past the back-off point
    s << set_reverse_software_limit(Axis::Reservoir, 0);
*/


    return s.str();
}

std::vector<double> Printer::get_last_bed_temp_list() {
    char buff[G_HUGE_BUFFER];
    GArrayUpload(mcu->g, "BEDTEMPS", G_BOUNDS, G_BOUNDS, G_COMMA, buff, G_HUGE_BUFFER);
    std::stringstream s;
    s.str(buff);
    std::string segment;
    std::vector<double> bedTempList;

    while(std::getline(s, segment, ',')) {
        double temperature = stod(segment)/40.96;
        if(temperature != 0) {
            bedTempList.push_back(temperature);
            qDebug(std::to_string(bedTempList.back()).c_str());
        }
    }
    return bedTempList;
}

std::string Printer::cure_layer(const PrintParameters &settings)
{
    double yAxisTraverseSpeed_mm_s {30};
    double heatLampStart_mm {-180};
    double heatLampEnd_mm {-330};
    double pyrometerPosition_mm {-249}; //start at -249, end at -339
    
    std::stringstream ss;

    ss << CMD::display_message("curing layer");

    //get last temperature
    std::vector<double> bedTempList = get_last_bed_temp_list();
    double averageTemp{0};

    if (bedTempList.size() != 0) {
        double sum = std::accumulate(bedTempList.begin(), bedTempList.end(), 0.0);
        averageTemp = (sum/bedTempList.size());
        heatLamp -> set_last_temp(averageTemp);
    }
    ss << CMD::display_message("last temperature was: " + std::to_string(averageTemp));

    double zAxisOffsetUnderRoller {0.5};

    // move z-axis down when going back to get more powder
    ss << CMD::set_accleration(Axis::Z, 10)
       << CMD::set_deceleration(Axis::Z, 10)
       << CMD::set_speed(Axis::Z, 2)
       << CMD::position_relative(Axis::Z, -zAxisOffsetUnderRoller)
       << CMD::begin_motion(Axis::Z)
       << CMD::motion_complete(Axis::Z);

    //move to edge of heat lamp
    ss << CMD::set_deceleration(Axis::Y, 1000);
    ss << CMD::set_accleration(Axis::Y, 1000);
    ss << CMD::set_speed(Axis::Y, yAxisTraverseSpeed_mm_s);
    ss << CMD::position_absolute(Axis::Y, heatLampStart_mm);
    ss << CMD::begin_motion(Axis::Y);
    ss << CMD::motion_complete(Axis::Y);

    //turn on heat lamp
    heatLamp -> target_temp = settings.target_temp;
    heatLamp -> kp = settings.kp;
    heatLamp -> ki = settings.ki;
    heatLamp -> kd = settings.kd;
    heatLamp -> starting_intensity = settings.starting_intensity;
    heatLamp -> default_intensity = settings.default_intensity;
    double next_intensity = heatLamp -> get_next_intensity();
    int int_intensity = (int) next_intensity;
    double duty_cycle {next_intensity - int_intensity};
    int period_ms {1000};
    ss << CMD::display_message("set intensity to: " + std::to_string(next_intensity));
    std::string program = "i = 0\n#Loop\n";
    program += CMD::cmd_buf_to_dmc(std::stringstream(heatLamp -> set_intensity(int_intensity + 1)));
    program += "\nWT " + std::to_string((int) ((duty_cycle)*period_ms)) + "\n";
    program += CMD::cmd_buf_to_dmc(std::stringstream(heatLamp -> set_intensity(int_intensity)));
    program += "\nWT " + std::to_string((int) ((1 - duty_cycle)*period_ms)) + "\n";
    program += "i = i + 1\n";
    program += "JP #Loop, i < " + std::to_string((int) ceil((settings.cureTime_s*1000 + settings.waitAfterHeatLampOn_millisecs)/period_ms)) + "\n";
    program += CMD::cmd_buf_to_dmc(std::stringstream(heatLamp -> set_intensity(0)));
    program += "\nEN";
    int rc = GProgramDownload(mcu->g, program.c_str(), "");
    if (rc != G_NO_ERROR) {
        qDebug(("Program not downloaded, error:" + std::to_string(rc)).c_str());
    }
    ss << "GCmd," << "XQ" << "\n";
    ss << CMD::sleep(settings.waitAfterHeatLampOn_millisecs);

    //move to pyrometer position
    double cureSpeed_mm_s = abs(heatLampStart_mm - heatLampEnd_mm)/settings.cureTime_s;
    ss << CMD::set_speed(Axis::Y, cureSpeed_mm_s);
    ss << CMD::position_absolute(Axis::Y, pyrometerPosition_mm);
    ss << CMD::begin_motion(Axis::Y);
    ss << CMD::motion_complete(Axis::Y);

    //start recording temperature
    std::string emptyarray = "0";
    for(int i = 1; i < 1000; i++) {
        emptyarray += ",0";
    }
    GArrayDownload(mcu->g, "BEDTEMPS", 0, 999, emptyarray.c_str());
    ss << CMD::record_array_mode("BEDTEMPS");
    ss << CMD::record_analog_data(1);
    ss << CMD::start_recording(100);

    ss << CMD::detail::GCmd() + "BEDTEMP[0] = @AN[1] \n";

    //move to other end of heat lamp
    ss << CMD::set_speed(Axis::Y, cureSpeed_mm_s);
    ss << CMD::position_absolute(Axis::Y, heatLampEnd_mm);
    ss << CMD::begin_motion(Axis::Y);
    ss << CMD::motion_complete(Axis::Y);

    //move to end of pyrometer
    ss << CMD::set_speed(Axis::Y, cureSpeed_mm_s);
    ss << CMD::position_absolute(Axis::Y, pyrometerPosition_mm - 90);
    ss << CMD::begin_motion(Axis::Y);
    ss << CMD::motion_complete(Axis::Y);
    ss << CMD::stop_recording();

    //turn off heat lamp (it should already be off by this point, just to make sure
    ss << heatLamp -> set_intensity(0);

    //move up to original z position
    ss << CMD::position_relative(Axis::Z, zAxisOffsetUnderRoller)
       << CMD::begin_motion(Axis::Z)
       << CMD::motion_complete(Axis::Z);

    ss << CMD::display_message("done curing layer");

    return ss.str();
}

std::string CMD::spread_layer(const RecoatSettings &settings)
{
    std::stringstream s;
    Axis y {Axis::Y};
    double zAxisOffsetUnderRoller {0.5};

    // move z-axis down when going back to get more powder
    s << set_accleration(Axis::Z, 10)
      << set_deceleration(Axis::Z, 10)
      << set_speed(Axis::Z, 2)
      << position_relative(Axis::Z, -zAxisOffsetUnderRoller)
      << begin_motion(Axis::Z)
      << motion_complete(Axis::Z);

    // jog y-axis to back
    s << set_accleration(y, 400)
      << set_deceleration(y, 400)
      << set_jog(y, -50)
      << begin_motion(y)
      << motion_complete(y);

    // set z-axis move distance
    if (settings.isLevelRecoat) // move z-axis back up all the way
        s << position_relative(Axis::Z, zAxisOffsetUnderRoller);
    else // move up but a layer thickness down from original position
        s << position_relative(Axis::Z,
                               zAxisOffsetUnderRoller
                               - (settings.layerHeight_microns
                                  / 1000.0));

    // set hopper settings
    s << set_hopper_mode_and_intensity(settings.ultrasonicMode,
                                       settings.ultrasonicIntensityLevel);

    // move z-axis
    s << begin_motion(Axis::Z);
    s << motion_complete(Axis::Z);

    // turn on hopper
    s << enable_hopper();

    // wait
    s << sleep(settings.waitAfterHopperOn_millisecs);

    // move y-axis forward
    s << set_deceleration(y, 1000);
    s << set_accleration(y, 1000);
    s << set_speed(y, settings.recoatSpeed_mm_s);
    s << position_relative(y, 100);
    s << begin_motion(y);
    s << motion_complete(y);

    // turn off hopper and enable rollers
    s << disable_hopper();

    s << enable_roller1();
    s << enable_roller2();

    // move y-axis forward under roller
    s << set_speed(y, settings.rollerTraverseSpeed_mm_s);
    s << position_relative(y, 175);
    s << begin_motion(y);
    s << motion_complete(y);

    // turn off rollers
    s << disable_roller1();
    s << disable_roller2();

    return s.str();
}

std::string CMD::sift_powder(int ultrasonicMode, int ultrasonicIntensity, int duration_ms)
{
    std::stringstream s;

    // Display a message on the printer's screen
    s << CMD::display_message("Sifting powder...");

    // Set the ultrasonic mode and intensity based on UI settings
    s << set_hopper_mode_and_intensity(ultrasonicMode, ultrasonicIntensity);

    // Turn on the hopper/ultrasonics
    s << enable_hopper();

    // Wait for the specified duration
    s << sleep(duration_ms);

    // Turn off the hopper/ultrasonics
    s << disable_hopper();

    // Display a completion message and then clear the line
    s << CMD::display_message("Sifting complete.");
    s << CMD::display_message("");

    return s.str();
}
std::string CMD::detail::create_gcmd(
        std::string_view command,
        Axis axis,
        int quantity)
{
    std::string result;
    result += GCmd();
    result += command;
    result += axis_string(axis);
    result += "=";
    result += std::to_string(quantity);
    result += "\n";
    return result;
}

std::string CMD::cmd_buf_to_dmc(const std::stringstream &s)
{
    std::stringstream ss;
    ss << s.rdbuf(); // copy to new string stream
    std::string returnString;
    std::string buffer;
    while (std::getline(ss, buffer)) // Reads whole line (includes spaces)
    {
        std::string delimeterChar = ",";
        size_t pos{0};
        std::string commandType;
        pos = buffer.find(delimeterChar);

        if (pos != std::string::npos)
        {
            commandType = buffer.substr(0, pos);
            buffer.erase(0, pos + delimeterChar.length());
        }
        else
        {
            commandType = buffer;
            buffer = "";
        }
        returnString += buffer;
        returnString += "\n";
    }

    return returnString;
}

double calculate_acceleration_distance(
        double speed_mm_per_s,
        double acceleration_mm_per_s2)
{
    // dx = v0*t + .5*a*t^2
    double accelerationTime = (speed_mm_per_s/acceleration_mm_per_s2);
    return 0.5 * acceleration_mm_per_s2 * std::pow(accelerationTime, 2);
}

// ======================================

CommandGenerator::CommandGenerator() {}

AxisSettings& CommandGenerator::settings(Axis axis)
{
    switch (axis)
    {
    case Axis::X:   return xAxisSettings;
    case Axis::Y:   return yAxisSettings;
    case Axis::Z:   return zAxisSettings;
    case Axis::Jet: return zAxisSettings;

    default: throw std::invalid_argument("invalid axis");
    }
}

// don't use this function yet...
// the default accelerations have not been set up yet
std::stringstream& CommandGenerator::jog_axis(Axis axis, double speed_mm_s)
{
    // I need to be able to get settings from the Axis...
    // settings(axis).acceleration
    // do I want to use default settings??
    // do I want to be able to change the defaults in the program??
    s << CMD::set_accleration(axis, settings(axis).acceleration);
    s << CMD::set_deceleration(axis, settings(axis).deceleration);
    s << CMD::set_jog(axis, speed_mm_s);
    s << CMD::begin_motion(axis);
    return s;
}

std::string CMD::quick_purge(int pulseTime_ms)
{
    std::stringstream s;
    s << CMD::display_message("Started quick purging valve.");
    // Turn valve ON
    s << CMD::set_bit(PURGE_VALVE_BIT);
    // Wait
    s << CMD::sleep(pulseTime_ms);
    // Turn valve OFF
    s << CMD::clear_bit(PURGE_VALVE_BIT);
    s << CMD::display_message("Finished quick purging valve.");

    return s.str();
}



#include "moc_printer.cpp"
