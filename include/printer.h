/*
 * Extended I/O 44 pin HD D-Sub Connector Pin Layout
 * Pin 1  - I/O Bit 18
 * Pin 2  - I/O Bit 21
 * Pin 3  - I/O Bit 24
 * Pin 4  - I/O Bit 26
 * Pin 5  - I/O Bit 29
 * Pin 6  - I/O Bit 32
 * Pin 7  - I/O Bit 33
 * Pin 8  - I/O Bit 36
 * Pin 9  - I/O Bit 38
 * Pin 10 - No Connect
 * Pin 11 - I/O Bit 41
 * Pin 12 - I/O Bit 44
 * Pin 13 - I/O Bit 47
 * Pin 14 - No Connect
 * Pin 15 - Reserved
 * Pin 16 - I/O Bit 17
 * Pin 17 - I/O Bit 20
 * Pin 18 - I/O Bit 23
 * Pin 19 - I/O Bit 25
 * Pin 20 - I/O Bit 28
 * Pin 21 - I/O Bit 31
 * Pin 22 - No Connect
 * Pin 23 - I/O Bit 35
 * Pin 24 - I/O Bit 37
 * Pin 25 - No Connect
 * Pin 26 - I/O Bit 40
 * Pin 27 - I/O Bit 43
 * Pin 28 - I/O Bit 46
 * Pin 29 - I/O Bit 48
 * Pin 30 - +3.3V
 * Pin 31 - I/O Bit 19
 * Pin 32 - I/O Bit 22
 * Pin 33 - Digital Ground
 * Pin 34 - I/O Bit 34
 * Pin 35 - No Connect
 * Pin 36 - Digital Ground
 * Pin 37 - I/O Bit 34
 * Pin 38 - No Connect
 * Pin 39 - Digital Ground
 * Pin 40 - I/O Bit 39
 * Pin 41 - I/O Bit 42
 * Pin 42 - I/O Bit 45
 * Pin 43 - Digital Ground
 * Pin 44 - No Connect
 *
 * Extended I/O Banks:
 * Banks of I/O can be configured using the "CO" command which takes a bitmask of banks
 * set bit to 1 to set bank as output, set bit to 0 for input
 * Bit # | IO Bank | IO Points
 *   7   |  N/A    |   N/A
 *   6   |  N/A    |   N/A
 *   5   |  N/A    |   N/A
 *   4   |  N/A    |   N/A
 *   3   |  Bank 5 |   41-48
 *   2   |  Bank 4 |   33-40
 *   1   |  Bank 3 |   25-32
 *   0   |  Bank 2 |   17-24
 *
 * 00001111 = 15 = all banks set as outputs
 * 00000001 = 1  = bank 2 set as outputs
 */

#ifndef PRINTER_H
#define PRINTER_H
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"
#include <string>
#include <sstream>
#include <string_view>
#include <functional>
#include <map>
#include <QObject>

class PrintThread;
class GInterruptHandler;
namespace PCD { class Controller; }
namespace JetDrive { class Controller; }
namespace Mister { class Controller; }
class DMC4080;

// TODO: get rid of these #defines and convert to
// constants in a namespace or static members
#define X_CNTS_PER_MM 1000
#define Y_CNTS_PER_MM 800
#define Z_CNTS_PER_MM 75745.7108f

#define X_STAGE_LEN_MM 150
#define Y_STAGE_LEN_MM 500
#define Z_STAGE_LEN_MM 15

#define PRINT_X_SIZE_MM 100
#define PRINT_Y_SIZE_MM 100
#define PRINT_Z_SIZE_MM 14 //?

// NOTE: these are not the same as the pin number on the DSUB HD44 cable
#define ROLLER_1_BIT 18 // pin 1
#define ROLLER_2_BIT 20 // pin 17

#define HEAT_LAMP_BIT 24 // pin 3

#define HS_TTL_BIT 17 // pin 16
#define MISTER_BIT 21 // pin 2

enum class Axis
{
    X, Y, Z, Jet
};

enum class MotorType
{
    Servo, Servo_R, StepLow, StepLow_R, StepHigh, StepHigh_R, Servo2PB, Servo2PB_R
};

enum Interrupt
{
    X_MOTION_COMPLETE = 0xD0,
    Y_MOTION_COMPLETE = 0xD1,
    Z_MOTION_COMPLETE = 0xD2,
    D_MOTION_COMPLETE = 0xD3,
    E_MOTION_COMPLETE = 0xD4,
    F_MOTION_COMPLETE = 0xD5,
    G_MOTION_COMPLETE = 0xD6,
    JETTING_COMPLETE = 0xD7,
    ALL_MOTION_COMPLETE = 0xD8,
    EXCESS_POSITION_ERROR = 0xC8,
    LIMIT_SWITCH = 0xC0,
    PROGRAM_STOPPED = 0xDB
    // there are a few more
};

inline std::string interrupt_string(Interrupt interrupt)
{
    switch (interrupt)
    {
    case Interrupt::X_MOTION_COMPLETE:
        return "X motion complete";
    case Interrupt::Y_MOTION_COMPLETE:
        return "Y motion complete";
    case Interrupt::Z_MOTION_COMPLETE:
        return "Z motion complete";
    case Interrupt::JETTING_COMPLETE:
        return "Jetting Stopped";
    case Interrupt::ALL_MOTION_COMPLETE:
        return "All motion complete";
    case Interrupt::PROGRAM_STOPPED:
        return "Program stopped";
    default:
        return "Interrupt not recognized";
    }
}

class Printer : public QObject
{
    Q_OBJECT

public:
    explicit Printer(QObject *parent = nullptr);
    ~Printer();

    static float motor_type_value(MotorType motorType);

public:

    DMC4080 *mcu {nullptr};
    JetDrive::Controller *jetDrive {nullptr};
    PCD::Controller *pressureController {nullptr};
    Mister::Controller *mister {nullptr};


    // TODO:
    // printer should own handles to the USB Camera

};

struct RecoatSettings
{
    // Should I be using unsigned vales here??
    double recoatSpeed_mm_s {100};
    double rollerTraverseSpeed_mm_s {3};
    // should this be another type? Maybe another enum?
    int ultrasonicIntensityLevel {25};
    int ultrasonicMode {0};
    int layerHeight_microns {35};
    bool isLevelRecoat {false};

    int waitAfterHopperOn_millisecs {1000};
    double yJogSpeedToHopper_mm_s {30};
    // these are relative positions... should they be absolute positions?
    double recoatYAxisTravel {115};
    double rollerYAxisTravel {172.5};

    // These are not just recoat settings,
    // but are values that are used when spreading a new layer
    // Decide where to put these
    double xAxisDefaultAcceleration {};
    double yAxisDefaultAcceleration {};
    double zAxisDefaultAcceleration {};
};

double calculate_acceleration_distance(
        double speed_mm_per_s,
        double acceleration_mm_per_s2);

namespace CMD
{
using std::string;
namespace detail
{
string axis_string(Axis axis);
constexpr int mm2cnts(double mm, Axis axis);
string create_gcmd(std::string_view command, Axis axis, int quantity);

inline string to_ASCII_code(char charToConvert)
{ return "{^" + std::to_string(int(charToConvert)) + "}, "; }

// TODO: A lot of these don't need to be functions
// but I want to rebuild this anyway
inline string GCmd() { return "GCmd,"; }
inline string GCmd(std::string_view command)
{ return GCmd() + command.data() + "\n"; }
inline string GCmdInt() { return "GCmdInt,"; }
inline string GMotionComplete() { return "GMotionComplete,"; }
inline string JetDrive() { return "JetDrive,"; }
inline string GSleep() { return "GSleep,"; }
inline string Message() { return "Message,"; }
inline string GOpen() { return "GOpen"; }
}

string set_default_controller_settings();
string cmd_buf_to_dmc(const std::stringstream &s);
string homing_sequence(bool homeZAxis);
string move_xy_axes_to_default_position();
string add_pvt_data_to_buffer(Axis axis,
                              double relativePosition_mm,
                              double velocity_mm,
                              int time_counts);
string exit_pvt_mode(Axis axis);
string begin_pvt_motion(Axis axis);
string set_hopper_mode_and_intensity(int mode, int intensity);
string set_jetting_gearing_ratio_from_droplet_spacing(Axis masterAxis,
                                                      int dropletSpacing);
string mist_layer(double traverseSpeed_mm_per_s, int sleepTime_ms);
string spread_layer(const RecoatSettings &settings);

// Establish connection with motion controller
inline string open_connection_to_controller()
{ return detail::GOpen() + "\n"; }

// The Acceleration command (AC) sets the linear acceleration
// of the motors for independent moves, such as PR, PA, and JG moves.
// The parameters will be rounded down to the nearest factor of 1024
// and have units of counts per second squared.
inline string set_accleration(Axis axis, double speed_mm_s2)
{ return detail::create_gcmd("AC", axis, detail::mm2cnts(speed_mm_s2, axis)); }

// The Deceleration command (DC) sets the linear deceleration
// of the motors for independent moves such as PR, PA, and JG moves.
// The parameters will be rounded down to the nearest factor of 1024
// and have units of counts per second squared.
inline string set_deceleration(Axis axis, double speed_mm_s2)
{ return detail::create_gcmd("DC", axis, detail::mm2cnts(speed_mm_s2, axis)); }

// The Limit Switch Deceleration command (SD) sets the linear deceleration rate
// of the motors when a limit switch has been reached.
inline string set_limit_switch_deceleration(Axis axis, double speed_mm_s2)
{ return detail::create_gcmd("SD", axis, detail::mm2cnts(speed_mm_s2, axis)); }

// The SP command sets the slew speed of any or all axes
// for independent moves.
inline string set_speed(Axis axis, double speed_mm_s)
{ return detail::create_gcmd("SP", axis, detail::mm2cnts(speed_mm_s, axis)); }

// The JG command sets the jog mode and the jog slew speed of the axes.
inline string set_jog(Axis axis, double speed_mm_s)
{ return detail::create_gcmd("JG", axis, detail::mm2cnts(speed_mm_s, axis)); }

// Sets the slew speed for the FI final move to the index and all but the first stage of HM.
inline string set_homing_velocity(Axis axis, double velocity_mm_s)
{ return detail::create_gcmd("HV", axis, detail::mm2cnts(velocity_mm_s, axis)); }

// The FL command sets the forward software position limit.
// If this limit is exceeded during motion, motion on that axis will decelerate to a stop.
// Forward motion beyond this limit is not permitted.
inline string set_forward_software_limit(Axis axis, double position_mm)
{ return detail::create_gcmd("FL", axis, detail::mm2cnts(position_mm, axis)); }

// The PR command sets the incremental distance and direction of the next move.
// The move is referenced with respect to the current position.
inline string position_relative(Axis axis, double relativePosition_mm)
{ return detail::create_gcmd("PR", axis, detail::mm2cnts(relativePosition_mm, axis)); }

// The PA command sets the end target of the Position Absolute Mode of Motion.
inline string position_absolute(Axis axis, double absolutePosition_mm)
{ return detail::create_gcmd("PA", axis, detail::mm2cnts(absolutePosition_mm, axis)); }

// The DP command sets the current motor position and current command positions to a user specified value.
// The units are in quadrature counts. This command will set both the TP and RP values.
// The DP command sets the commanded reference position for axes configured as steppers. The units are in steps.
// Example: "DP 0" This will set the registers for TD and RP to zero, but will not effect the TP register value.
//          When equipped with an encoder, use the DE command to set the encoder position for stepper mode.
inline string define_position(Axis axis, double position_mm)
{ return detail::create_gcmd("DP", axis, detail::mm2cnts(position_mm, axis)); }

// The BG command starts a motion on the specified axis or sequence.
inline string begin_motion(Axis axis)
{ return detail::GCmd() + "BG" + detail::axis_string(axis) + "\n"; }

inline string motion_complete(Axis axis)
{ return detail::GMotionComplete() + detail::axis_string(axis) + "\n"; }

inline string sleep(int milliseconds)
{ return detail::GSleep() + std::to_string(milliseconds) + "\n";}

// The FI and BG commands move the motor until an encoder index pulse is detected.
inline string find_index(Axis axis)
{ return detail::GCmd() + "FI" + detail::axis_string(axis) + "\n"; }

// The SH commands tells the controller to use the current motor position
// as the command position and to enable servo control at the current position.
inline string servo_here(Axis axis)
{ return detail::GCmd() + "SH" + detail::axis_string(axis) + "\n"; }

// The ST command stops motion on the specified axis. Motors will come to a decelerated stop.
inline string stop_motion(Axis axis)
{ return detail::GCmd() + "ST" + detail::axis_string(axis) + "\n"; }

// The SB command sets a particular digital output. The SB and CB (Clear Bit)
// instructions can be used to control the state of output lines.
inline string set_bit(int bit)
{ return detail::GCmd() + "SB " + std::to_string(bit) + "\n"; }

// The CB command clears a particular digital output.
// The SB and CB (Clear Bit) instructions can be used to control the state of output lines.
inline string clear_bit(int bit)
{ return detail::GCmd() + "CB " + std::to_string(bit) + "\n"; }

inline string enable_roller1() { return set_bit(ROLLER_1_BIT); }
inline string disable_roller1() { return clear_bit(ROLLER_1_BIT); }

inline string enable_roller2() { return set_bit(ROLLER_2_BIT); }
inline string disable_roller2() { return clear_bit(ROLLER_2_BIT); }

// 'U1' sent to the generator over serial port 2. 49 is the ASCII code for '1'
// TODO: Try "MG{P2} U1\r"
inline string enable_hopper()
{ return detail::GCmd() + "MG{P2} {^85}, {^49}, {^13}{N}" + "\n";}
// 'U0' sent to the generator over serial port 2. 49 is the ASCII code for '1'
inline string disable_hopper()
{ return detail::GCmd() + "MG{P2} {^85}, {^48}, {^13}{N}" + "\n"; }

inline string disable_forward_software_limit(Axis axis)
{ return detail::create_gcmd("FL", axis, 2147483647); }

// this command does not support newlines or commas right now...
// either will cause the print to fail...
inline string display_message(const std::string &message)
{ return detail::Message() + message + "\n"; }

inline string enable_gearing_for(Axis slaveAxis, Axis masterAxis)
{
    return detail::GCmd()
            + "GA"
            + detail::axis_string(slaveAxis)
            + "="
            + detail::axis_string(masterAxis)
            + "\n";
}

inline string disable_gearing_for(Axis slaveAxis)
{
    return detail::GCmd()
            + "GR"
            + detail::axis_string(slaveAxis)
            + "="
            + "0"
            + "\n";
}

// === These trippoint commands don't work through gclib... ===
// don't use unless uploading these commands to the controller directly
string at_time_samples(int samples);
string at_time_milliseconds(int milliseconds);
string after_absolute_position(Axis axis, double absolutePosition_mm);

inline string set_reference_time()
{ return detail::GCmd() + "AT 0" + "\n"; }
inline string after_motion(Axis axis)
{ return detail::GCmd() + "AM " + detail::axis_string(axis) + "\n";}
inline string wait(int milliseconds)
{ return detail::GCmd() + "WT " + std::to_string(milliseconds) + "\n"; }
// =====================================================================

} // end CMD namespace

// === Stuff I'm Working on ===

struct AxisSettings
{
    // I want these to be constant
    // double counts_per_mm;
    // double stageLength_mm;
    double speed;
    double acceleration;
    double deceleration;
};

class CommandGenerator
{
public:
    explicit CommandGenerator();
    std::stringstream& jog_axis(Axis axis, double speed_mm_s);
    void clear_command_buffer();

private:
    std::stringstream s;

    AxisSettings& settings(Axis axis);
    // how do I want to handle defaults??
    // PrinterSettings settings_?
    // do I make this public or private?
    RecoatSettings recoatSettings;
    AxisSettings xAxisSettings;
    AxisSettings yAxisSettings;
    AxisSettings zAxisSettings;
};

#endif // PRINTER_H
