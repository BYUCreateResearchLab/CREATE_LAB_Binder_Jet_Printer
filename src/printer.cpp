#include "printer.h"

#include <sstream>
#include <cmath>
#include <stdexcept>

GReturn GCALL GProgramComplete(GCon g)
{
    char pred[] = "_XQ=-1"; //predicate for polling the axis' motion status, m is a place holder replaced below.
       GReturn rc;

       rc = GWaitForBool(g, pred, -1); // poll forever. Change this if a premature exit is desired.
       if (rc != G_NO_ERROR)
           return rc;

       return G_NO_ERROR;
}

double calculate_acceleration_distance(double speed_mm_per_s, double acceleration_mm_per_s2)
{
    // dx = v0*t + .5*a*t^2
    double accelerationTime = (speed_mm_per_s/acceleration_mm_per_s2);
    return 0.5 * acceleration_mm_per_s2 * std::pow(accelerationTime, 2);
}

using namespace CMD::detail;

Printer::Printer()
{

}

CommandGenerator::CommandGenerator()
{

}

AxisSettings& CommandGenerator::settings(Axis axis)
{
    switch (axis)
    {
    case Axis::X:   return xAxisSettings;
    case Axis::Y:   return yAxisSettings;
    case Axis::Z:   return zAxisSettings;
    case Axis::Jet: return zAxisSettings;

    default:
        throw std::invalid_argument("invalid axis");
    }
}

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

int CMD::detail::mm2cnts(double mm, Axis axis)
{
    switch (axis)
    {
    case Axis::X:   return (int)(mm * X_CNTS_PER_MM);
    case Axis::Y:   return (int)(mm * Y_CNTS_PER_MM);
    case Axis::Z:   return (int)(mm * Z_CNTS_PER_MM);
    case Axis::Jet: return (int)(mm);

    default:
        throw std::invalid_argument("invalid axis");
    }
}

int CMD::detail::um2cnts(double um, Axis axis)
{
    return mm2cnts(um / 1000.0, axis);
}

std::string CMD::detail::GCmd()
{
    return {"GCmd,"};
}

std::string CMD::detail::GCmdInt() // returns an int for commands like TP, RP, TE, etc.
{
    return {"GCmdInt,"};
}

std::string CMD::detail::GMotionComplete()
{
    return {"GMotionComplete,"};
}

std::string CMD::detail::JetDrive()
{
    return {"JetDrive,"};
}

std::string CMD::detail::GSleep()
{
    return {"GSleep,"};
}

std::string CMD::detail::Message()
{
    return {"Message,"};
}

std::string CMD::detail::GOpen()
{
    return {"GOpen"};
}

// The Acceleration command (AC) sets the linear acceleration
// of the motors for independent moves, such as PR, PA, and JG moves.
// The parameters will be rounded down to the nearest factor of 1024
// and have units of counts per second squared.
std::string CMD::set_accleration(Axis axis, double speed_mm_s2)
{
    return create_gcmd("AC", axis, mm2cnts(speed_mm_s2, axis));
}

// The Deceleration command (DC) sets the linear deceleration
// of the motors for independent moves such as PR, PA, and JG moves.
// The parameters will be rounded down to the nearest factor of 1024
// and have units of counts per second squared.
std::string CMD::set_deceleration(Axis axis, double speed_mm_s2)
{
    return {create_gcmd("DC", axis, mm2cnts(speed_mm_s2, axis))};
}

// The Limit Switch Deceleration command (SD) sets the linear deceleration rate
// of the motors when a limit switch has been reached.
std::string CMD::set_limit_switch_deceleration(Axis axis, double speed_mm_s2)
{
    return {create_gcmd("SD", axis, mm2cnts(speed_mm_s2, axis))};
}

// The SP command sets the slew speed of any or all axes
// for independent moves.
std::string CMD::set_speed(Axis axis, double speed_mm_s)
{
    return {create_gcmd("SP", axis, mm2cnts(speed_mm_s, axis))};
}

std::string CMD::set_jog(Axis axis, double speed_mm_s)
{
    return {create_gcmd("JG", axis, mm2cnts(speed_mm_s, axis))};
}

std::string CMD::set_homing_velocity(Axis axis, double velocity_mm_s)
{
   return {create_gcmd("HV", axis, mm2cnts(velocity_mm_s, axis))};
}

std::string CMD::set_forward_software_limit(Axis axis, double position_mm)
{
    return {create_gcmd("FL", axis, mm2cnts(position_mm, axis))};
}

std::string CMD::position_relative(Axis axis, double relativePosition_mm)
{
    return {create_gcmd("PR", axis, mm2cnts(relativePosition_mm, axis))};
}

std::string CMD::position_absolute(Axis axis, double absolutePosition_mm)
{
    return {create_gcmd("PA", axis, mm2cnts(absolutePosition_mm, axis))};
}

std::string CMD::add_pvt_data_to_buffer(Axis axis, double relativePosition_mm, double velocity_mm, int time_counts)
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

std::string CMD::define_position(Axis axis, double position_mm)
{
   return {create_gcmd("DP", axis, mm2cnts(position_mm, axis))};
}

std::string CMD::begin_motion(Axis axis)
{
    return {GCmd() + "BG" + detail::axis_string(axis) + "\n"};
}

std::string CMD::motion_complete(Axis axis)
{
    return {GMotionComplete() + axis_string(axis) + "\n"};
}

std::string CMD::sleep(int milliseconds)
{
    return {GSleep() + std::to_string(milliseconds) + "\n"};
}

std::string CMD::find_index(Axis axis)
{
    return {GCmd() + "FI" + detail::axis_string(axis) + "\n"};
}

std::string CMD::servo_here(Axis axis)
{
    return {GCmd() + "SH" + detail::axis_string(axis) + "\n"};
}

std::string CMD::stop_motion(Axis axis)
{
    return {GCmd() + "ST" + detail::axis_string(axis) + "\n"};
}

std::string CMD::set_reference_time()
{
    return {GCmd() + "AT 0" + "\n"};
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

std::string CMD::after_motion(Axis axis)
{
    return {GCmd() + "AM " + detail::axis_string(axis) + "\n"};
}

std::string CMD::wait(int milliseconds)
{
    return {GCmd() + "WT " + std::to_string(milliseconds) + "\n"};
}

std::string CMD::set_bit(int bit)
{
    return {GCmd() + "SB " + std::to_string(bit) + "\n"};
}

std::string CMD::clear_bit(int bit)
{
    return {GCmd() + "CB " + std::to_string(bit) + "\n"};
}

std::string CMD::enable_roller1()
{
    // update this to just use the set_bit command
    return {GCmd() + "SB " + std::to_string(ROLLER_1_BIT) + "\n"};
}

std::string CMD::disable_roller1()
{
    return {GCmd() + "CB " + std::to_string(ROLLER_1_BIT) + "\n"};
}

std::string CMD::enable_roller2()
{
    return {GCmd() + "SB " + std::to_string(ROLLER_2_BIT) + "\n"};
}

std::string CMD::disable_roller2()
{
    return {GCmd() + "CB " + std::to_string(ROLLER_2_BIT) + "\n"};
}

std::string CMD::set_hopper_mode_and_intensity(int mode, int intensity)
{
    // modes A-H (int mode is index 0-7)
    // intensity 100%-30% (int intensity is index 0-7)
    // "MG{P2} {^77}, {^48}, {^53}, {^13}{N}" is the correct command for "M05" or Mode: 'A' and Intensity: 50%
    return GCmd() + "MG{P2} "
                  + to_ASCII_code('M')
                  + to_ASCII_code('0' + mode) // converts int to char and then gets backs a string of the ASCII code to send as a command to the generator
                  + to_ASCII_code('0' + intensity)
                  + "{^13}{N}" + "\n";
}

std::string CMD::enable_hopper()
{
    // 'U1' sent to the generator over serial port 2. 49 is the ASCII code for '1'
    return GCmd() + "MG{P2} {^85}, {^49}, {^13}{N}" + "\n";
}

std::string CMD::disable_hopper()
{
    // 'U0' sent to the generator over serial port 2. 49 is the ASCII code for '1'
    return GCmd() + "MG{P2} {^85}, {^48}, {^13}{N}" + "\n";
}

std::string CMD::disable_forward_software_limit(Axis axis)
{
    return {create_gcmd("FL", axis, 2147483647)};
}

// this command does not support newlines or commas right now...
// either will cause the print to fail...
std::string CMD::display_message(const std::string &message)
{
    return {Message() + message + "\n"};
}

std::string CMD::enable_gearing_for(Axis slaveAxis, Axis masterAxis)
{
    return {GCmd() + "GA" + detail::axis_string(slaveAxis) + "=" + detail::axis_string(masterAxis) + "\n"};
}

std::string CMD::disable_gearing_for(Axis slaveAxis)
{
    return {GCmd() + "GR" + detail::axis_string(slaveAxis) + "=" + "0" + "\n"};
}

std::string CMD::move_xy_axes_to_default_position()
{
    std::stringstream s;
    s << CMD::set_speed(Axis::X, 60);
    s << CMD::set_speed(Axis::Y, 40);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    s << CMD::position_absolute(Axis::Y, 0);
    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    return s.str();
}

std::string CMD::mist_layer(double traverseSpeed_mm_per_s)
{
    std::stringstream s;

    const int yAxisTravelSpeed_mm_per_s = 60;
    const double startPosition_mm = -340;
    const double endPosition_mm = -154;
    const int sleepTime_ms = 5000;
    const double zAxisOffsetUnderRoller{0.5};

    // setup
    s << CMD::display_message("Misting layer");
    s << CMD::clear_bit(MISTER_BIT); // just make sure that the mister is off
    s << CMD::set_accleration(Axis::Y, 600);
    s << CMD::set_deceleration(Axis::Y, 600);
    s << CMD::set_speed(Axis::Y, yAxisTravelSpeed_mm_per_s);  

    // move z-axis down when going back to avoid hitting the roller
    s << CMD::set_accleration(Axis::Z, 10);
    s << CMD::set_deceleration(Axis::Z, 10);
    s << CMD::set_speed(Axis::Z, 2);
    s << CMD::position_relative(Axis::Z, -zAxisOffsetUnderRoller);
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);
    //

    s << CMD::position_absolute(Axis::Y, startPosition_mm); // move y-axis to start misting position
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::Y);

    s << CMD::set_bit(MISTER_BIT); // turn on mister
    s << CMD::sleep(sleepTime_ms); // wait

    s << CMD::set_speed(Axis::Y, traverseSpeed_mm_per_s); // set traverse speed
    s << CMD::position_absolute(Axis::Y, endPosition_mm); // travel under mister and specified speed
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::Y);

    s << CMD::clear_bit(MISTER_BIT); // turn off mister

    // move z-axis back up
    s << CMD::position_relative(Axis::Z, zAxisOffsetUnderRoller);
    s << CMD::begin_motion(Axis::Z);

    // move forward
    s << CMD::set_speed(Axis::Y, yAxisTravelSpeed_mm_per_s);
    s << CMD::position_absolute(Axis::Y, -50);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);
    s << CMD::display_message("Misting complete");

    return s.str();
}

std::string CMD::set_jetting_gearing_ratio_from_droplet_spacing(Axis masterAxis, int dropletSpacing_um)
{
    double gearingRatio = (1000.0 / ((double)dropletSpacing_um * mm2cnts(1, masterAxis)));
    return {GCmd() + "GR" + detail::axis_string(Axis::Jet) + "=" + std::to_string(gearingRatio) + "\n"};
}

std::string CMD::open_connection_to_controller()
{
    return {GOpen() + "\n"};                // Establish connection with motion controller
}

std::string CMD::set_default_controller_settings()
{
    std::stringstream s;

    // Controller Configuration
    s << CMD::detail::GCmd() << "MO"              << "\n";   // Ensure motors are off for setup

    // X Axis
    s << CMD::detail::GCmd() << "MTX=-1"          << "\n";   // Set motor type to reversed brushless
    s << CMD::detail::GCmd() << "CEX=2"           << "\n";   // Set Encoder to reversed quadrature
    s << CMD::detail::GCmd() << "BMX=40000"       << "\n";   // Set magnetic pitch of linear motor
    s << CMD::detail::GCmd() << "AGX=1"           << "\n";   // Set amplifier gain
    s << CMD::detail::GCmd() << "AUX=9"           << "\n";   // Set current loop (based on inductance of motor)
    s << CMD::detail::GCmd() << "TLX=3"           << "\n";   // Set constant torque limit to 3V
    s << CMD::detail::GCmd() << "TKX=0"           << "\n";   // Disable peak torque setting for now

    // Set PID Settings
    s << CMD::detail::GCmd() << "KDX=250"         << "\n";   // Set Derivative
    s << CMD::detail::GCmd() << "KPX=40"          << "\n";   // Set Proportional
    s << CMD::detail::GCmd() << "KIX=2"           << "\n";   // Set Integral
    s << CMD::detail::GCmd() << "PLX=0.1"         << "\n";   // Set low-pass filter

    // Y Axis
    s << CMD::detail::GCmd() << "MTY=1"           << "\n";   // Set motor type to standard brushless
    s << CMD::detail::GCmd() << "CEY=0"           << "\n";   // Set Encoder to reversed quadrature??? (or is it?)
    s << CMD::detail::GCmd() << "BMY=2000"        << "\n";   // Set magnetic pitch of rotary motor
    s << CMD::detail::GCmd() << "AGY=1"           << "\n";   // Set amplifier gain
    s << CMD::detail::GCmd() << "AUY=11"          << "\n";   // Set current loop (based on inductance of motor)
    s << CMD::detail::GCmd() << "TLY=6"           << "\n";   // Set constant torque limit to 6V
    s << CMD::detail::GCmd() << "TKY=0"           << "\n";   // Disable peak torque setting for now
    // Set PID Settings
    s << CMD::detail::GCmd() << "KDY=500"         << "\n";   // Set Derivative
    s << CMD::detail::GCmd() << "KPY=70"          << "\n";   // Set Proportional
    s << CMD::detail::GCmd() << "KIY=1.7002"      << "\n";   // Set Integral

    // Z Axis
    s << CMD::detail::GCmd() << "MTZ=-2.5"        << "\n";   // Set motor type to standard brushless
    s << CMD::detail::GCmd() << "CEZ=14"          << "\n";   // Set Encoder to reversed quadrature
    s << CMD::detail::GCmd() << "AGZ=0"           << "\n";   // Set amplifier gain
    s << CMD::detail::GCmd() << "AUZ=9"           << "\n";   // Set current loop (based on inductance of motor)
    // Note: There might be more settings especially for this axis I might want to add later

    // H Axis (Jetting Axis)
    s << CMD::detail::GCmd() << "MTH=-2"          << "\n";   // Set jetting axis to be stepper motor with defualt low
    s << CMD::detail::GCmd() << "AGH=0"           << "\n";   // Set gain to lowest value
    s << CMD::detail::GCmd() << "LDH=3"           << "\n";   // Disable limit sensors for H axis
    s << CMD::detail::GCmd() << "KSH=0.25"        << "\n";   // Minimize filters on step signals
    s << CMD::detail::GCmd() << "ITH=1"           << "\n";   // Minimize filters on step signals

    s << CMD::detail::GCmd() << "CC 19200,0,1,0"  << "\n";   //AUX PORT FOR THE ULTRASONIC GENERATOR
    s << CMD::detail::GCmd() << "CN=-1"           << "\n";   // Set correct polarity for all limit switches
    s << CMD::detail::GCmd() << "BN"              << "\n";   // Save (burn) these settings to the controller just to be safe
    s << CMD::detail::GCmd() << "SH XYZ"          << "\n";   // Enable X,Y, and Z motors
    s << CMD::detail::GCmd() << "SH H"            << "\n";   // Servo the jetting axis

    // Configure Extended I/O
    s << CMD::detail::GCmd() << "CO 1"            << "\n"; // configures bank 2 as outputs on extended I/O (IO 17-24)

    return s.str();
}

std::string CMD::homing_sequence()
{
    std::stringstream s;

    // === Home the X-Axis using the central home sensor index pulse ===

    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::set_limit_switch_deceleration(Axis::X, 800);
    s << CMD::set_jog(Axis::X, -25); // jog towards rear limit

    s << CMD::set_accleration(Axis::Y, 400);
    s << CMD::set_deceleration(Axis::Y, 400);
    s << CMD::set_limit_switch_deceleration(Axis::Y, 600);
    s << CMD::set_jog(Axis::Y, 25); // jog towards front limit

    s << CMD::set_accleration(Axis::Z, 20);
    s << CMD::set_deceleration(Axis::Z, 20);
    s << CMD::set_limit_switch_deceleration(Axis::Z, 40);
    s << CMD::set_jog(Axis::Z, -2);                       // jog to bottom (MAX SPEED of 5mm/s!)
    s << CMD::disable_forward_software_limit(Axis::Z);    // turn off top software limit

    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::begin_motion(Axis::Z);

    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);

    s << CMD::sleep(1000);

    // home to center index on x axis
    s << CMD::set_jog(Axis::X, 30);
    s << CMD::set_homing_velocity(Axis::X, 0.5);
    s << CMD::find_index(Axis::X);

    //s << CMD::set_speed(Axis::Y, 40);
    //s << CMD::position_relative(Axis::Y, -200);

    // home y axis to nearest index
    s << CMD::set_jog(Axis::Y, -0.5);
    s << CMD::set_homing_velocity(Axis::Y, 0.25);
    s << CMD::find_index(Axis::Y);

    s << CMD::set_accleration(Axis::Z, 10);        // slower acceleration for going back up
    s << CMD::set_speed(Axis::Z, 2);
    s << CMD::position_relative(Axis::Z, 13.5322); // TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!

    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::begin_motion(Axis::Z);

    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);

    //s << CMD::set_speed(Axis::X, 50);
    //s << CMD::position_relative(Axis::X, -40);
    //s << CMD::begin_motion(Axis::X);
    //s << CMD::motion_complete(Axis::X);

    s << CMD::define_position(Axis::X, X_STAGE_LEN_MM / 2.0);
    s << CMD::define_position(Axis::Y, 0);
    s << CMD::define_position(Axis::Z, 0);
    s << CMD::set_forward_software_limit(Axis::Z, 0); // set software limit to current position

    return s.str();
}

std::string CMD::spread_layer(const RecoatSettings &settings)
{
    std::stringstream s;
    Axis y{Axis::Y};
    double zAxisOffsetUnderRoller{0.5};

    // move z-axis down when going back to get more powder
    s << CMD::set_accleration(Axis::Z, 10);
    s << CMD::set_deceleration(Axis::Z, 10);
    s << CMD::set_speed(Axis::Z, 2);
    s << CMD::position_relative(Axis::Z, -zAxisOffsetUnderRoller);
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    // jog y-axis to back
    s << CMD::set_accleration(y, 400);
    s << CMD::set_deceleration(y, 400);
    s << CMD::set_jog(y, -50);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    // move z-axis back up
    if(settings.isLevelRecoat)
    {
        s << CMD::position_relative(Axis::Z, zAxisOffsetUnderRoller);
    }
    else
    {
        s << CMD::position_relative(Axis::Z, zAxisOffsetUnderRoller - (settings.layerHeight_microns / 1000.0));
    }
    // set hopper settings here
    s << CMD::set_hopper_mode_and_intensity(settings.ultrasonicMode, settings.ultrasonicIntensityLevel);
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    // turn on hopper
    s << CMD::enable_hopper();

    // wait
    s << CMD::sleep(settings.waitAfterHopperOn_millisecs);

    // move y-axis forward
    s << CMD::set_speed(y, settings.recoatSpeed_mm_s);
    s << CMD::position_relative(y, 120);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    // turn off hopper and enable rollers
    s << CMD::disable_hopper();

    // move up to roller (NOT NEEDED WITH CURRENT ROLLER PLACEMENT)
    //s << CMD::set_speed(y, 20);
    //s << CMD::position_relative(y, 32.5); // move up to roller
    //s << CMD::begin_motion(y);
    //s << CMD::motion_complete(y);

    s << CMD::enable_roller1();
    s << CMD::enable_roller2();

    // move y-axis forward under roller
    s << CMD::set_speed(y, settings.rollerTraverseSpeed_mm_s);
    s << CMD::position_relative(y, 135);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    // turn off rollers
    s << CMD::disable_roller1();
    s << CMD::disable_roller2();

    return s.str();
}

std::string CMD::detail::to_ASCII_code(char charToConvert)
{
    return "{^" + std::to_string(int(charToConvert)) + "}, ";
}

std::string CMD::detail::create_gcmd(const std::string &command, Axis axis, int quantity)
{
    std::string result;
    result += GCmd();
    result += command;
    result += axis_string(axis);
    result += "=";
    result += std::to_string(quantity);
    result += "\n";
    return result;
    // return GCmd() + command + axis_string(axis) + "=" + std::to_string(quantity) + "\n";
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
