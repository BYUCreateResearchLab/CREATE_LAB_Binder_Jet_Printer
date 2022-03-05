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

#define X_CNTS_PER_MM 1000
#define Y_CNTS_PER_MM 800
#define Z_CNTS_PER_MM 75745.7108f

#define X_STAGE_LEN_MM 150
#define Y_STAGE_LEN_MM 500
#define Z_STAGE_LEN_MM 15

#define PRINT_X_SIZE_MM 100
#define PRINT_Y_SIZE_MM 100
#define PRINT_Z_SIZE_MM 14 //?

// NOTE: THESE ARE NOT THE SAME AS THE PIN NUM ON THE DSUB HD44 Cable
#define ROLLER_1_BIT 18
#define ROLLER_2_BIT 20

#define HS_TTL_BIT 17
#define MISTER_BIT 21

GReturn GCALL GProgramComplete(GCon g);

class Printer
{
public:
    Printer();

    char const *address = "192.168.42.100"; // IP address of motion controller
    GCon g{0};                              // Handle for connection to Galil Motion Controller
};

enum class Axis
{
    X,
    Y,
    Z,
    Jet
};

struct RecoatSettings
{
    // Should I be using unsigned vales here??
    double recoatSpeed_mm_s{100};
    double rollerTraverseSpeed_mm_s{3};
    // should this be another type? Maybe another enum?
    int ultrasonicIntensityLevel{25};
    int ultrasonicMode{0};
    int layerHeight_microns{35};
    bool isLevelRecoat{false};

    int waitAfterHopperOn_millisecs{1000};
    double yJogSpeedToHopper_mm_s{30};
    // these are relative positions... should they be absolute positions?
    double recoatYAxisTravel{115};
    double rollerYAxisTravel{172.5};

    // These are not just recoat settings, but are values that are used when spreading a new layer
    // Decide where to put these
    double xAxisDefaultAcceleration{};
    double yAxisDefaultAcceleration{};
    double zAxisDefaultAcceleration{};
};

double calculate_acceleration_distance(double speed_mm_per_s, double acceleration_mm_per_s2);

namespace CMD
{

std::string cmd_buf_to_dmc(const std::stringstream &s);

std::string open_connection_to_controller();
std::string set_default_controller_settings();
std::string homing_sequence();
std::string set_accleration(Axis axis, double speed_mm_s2);
std::string set_deceleration(Axis axis, double speed_mm_s2);
std::string set_limit_switch_deceleration(Axis axis, double speed_mm_s2);
std::string set_speed(Axis axis, double speed_mm_s);
std::string set_jog(Axis axis, double speed_mm_s);
std::string set_homing_velocity(Axis axis, double velocity_mm_s);
std::string set_forward_software_limit(Axis axis, double position_mm);
std::string position_relative(Axis axis, double relativePosition_mm);
std::string position_absolute(Axis axis, double absolutePosition_mm);
std::string add_pvt_data_to_buffer(Axis axis, double relativePosition_mm, double velocity_mm, int time_counts);
std::string exit_pvt_mode(Axis axis);
std::string begin_pvt_motion(Axis axis);
std::string define_position(Axis axis, double position_mm);
std::string begin_motion(Axis axis);
std::string motion_complete(Axis axis);
std::string sleep(int milliseconds);
std::string find_index(Axis axis);
std::string servo_here(Axis axis);
std::string stop_motion(Axis axis);

// These trippoint commands don't work through gclib...
std::string set_reference_time();
std::string at_time_samples(int samples);
std::string at_time_milliseconds(int milliseconds);
std::string after_absolute_position(Axis axis, double absolutePosition_mm);
std::string after_motion(Axis axis);
std::string wait(int milliseconds);

std::string set_bit(int bit);
std::string clear_bit(int bit);

std::string enable_roller1();
std::string disable_roller1();
std::string enable_roller2();
std::string disable_roller2();

std::string set_hopper_mode_and_intensity(int mode, int intensity);
std::string enable_hopper();
std::string disable_hopper();

std::string disable_forward_software_limit(Axis axis);

std::string display_message(const std::string &message);

std::string enable_gearing_for(Axis slaveAxis, Axis masterAxis);
std::string set_jetting_gearing_ratio_from_droplet_spacing(Axis masterAxis, int dropletSpacing);
std::string disable_gearing_for(Axis slaveAxis);

std::string mist_layer(double traverseSpeed_mm_per_s);
std::string spread_layer(const RecoatSettings &settings);

namespace detail
{
std::string axis_string(Axis axis);

int mm2cnts(double mm, Axis axis);
int um2cnts(double um, Axis axis);

std::string to_ASCII_code(char charToConvert);
std::string create_gcmd(const std::string &command, Axis axis, int quantity);

std::string GCmd();
std::string GCmdInt();
std::string GMotionComplete();
std::string JetDrive();
std::string GSleep();
std::string Message();
std::string GOpen();
}

}

struct AxisSettings
{
    // I want these to be constant
    //double counts_per_mm;
    //double stageLength_mm;
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
