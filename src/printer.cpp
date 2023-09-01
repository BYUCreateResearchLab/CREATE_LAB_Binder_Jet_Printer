#include "printer.h"

#include <sstream>
#include <cmath>
#include <stdexcept>

#include "pcd.h"
#include "jetdrive.h"
#include "dmc4080.h"
#include "mister.h"

Printer::Printer(QObject *parent) :
    QObject(parent),
    mcu ( new DMC4080("192.168.42.100", this) ),
    jetDrive ( new JetDrive::Controller("COM8", this) ),
    pressureController ( new PCD::Controller("COM3", this) ),
    mister ( new Mister::Controller("COM4", this) )
{

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
    pressureController->connect_to_pressure_controller();
    // this isn't ready yet
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
      << GCmd("CEZ=14")      // Set encoder to reversed quadrature
      << GCmd("AGZ=0")       // Set amplifier gain
      << GCmd("AUZ=9")       // Set current loop (based on inductance of motor)
         // Note: There might be more settings especially for this axis I might want to add later

         // H Axis (Jetting Axis)
      << GCmd("MTH=-2")      // Set jetting axis to be stepper motor with defualt low
      << GCmd("AGH=0")       // Set gain to lowest value
      << GCmd("LDH=3")       // Disable limit sensors for H axis
      << GCmd("KSH=0.5")     // Minimize filters on step signals (0.25 when TM=1000)
      << GCmd("ITH=1" )      // Minimize filters on step signals
      << GCmd("YAH=1")       // set step resolution to 1 full step per step

         // Configure Extended I/O
      << GCmd("CO 1")        // configures bank 2 as outputs on extended I/O (IO 17-24)

      << GCmd("CC 19200,0,1,0")  // AUX PORT FOR THE ULTRASONIC GENERATOR
      << GCmd("CN=-1")           // Set correct polarity for all limit switches
      << GCmd("BN")              // Save (burn) these settings to the controller just to be safe
      << GCmd("SH XYZ")          // Enable X,Y, and Z motors
      << GCmd("SH H");           // Servo the jetting axis

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
    const double startPosition_mm = -340;
    const double endPosition_mm = -154;
    const double zAxisOffsetUnderRoller{0.5};

    // setup
    s << message("Misting layer");
    s << message("CMD MIST_OFF"); // just make sure that the mister is off
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
        s << disable_forward_software_limit(Axis::Z);
    }

    s << begin_motion(Axis::X);
    s << begin_motion(Axis::Y);
    if (homeZAxis)
        s << begin_motion(Axis::Z);

    s << motion_complete(Axis::X);
    s << motion_complete(Axis::Y);

    // TODO: This is a temporary solution that depends on alignment
    // of ballscrew and motor index pulse
    // find a better way to do this
    // move y-axis forward a bit to avoid being right on top of the index pulse
    s << position_relative(Axis::Y, -2);
    s << set_speed(Axis::Y, 10);
    s << begin_motion(Axis::Y);
    s << motion_complete(Axis::Y);
    // =================================

    // this is put after the short y move because the z axis is slow
    if (homeZAxis)
        s << motion_complete(Axis::Z);

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
    // set software limit to current position
    s << set_forward_software_limit(Axis::Z, 0);

    return s.str();
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
    s << set_speed(y, settings.recoatSpeed_mm_s);
    s << position_relative(y, 120);
    s << begin_motion(y);
    s << motion_complete(y);

    // turn off hopper and enable rollers
    s << disable_hopper();

    s << enable_roller1();
    s << enable_roller2();

    // move y-axis forward under roller
    s << set_speed(y, settings.rollerTraverseSpeed_mm_s);
    s << position_relative(y, 135);
    s << begin_motion(y);
    s << motion_complete(y);

    // turn off rollers
    s << disable_roller1();
    s << disable_roller2();

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

#include "moc_printer.cpp"
