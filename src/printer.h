#ifndef PRINTER_H
#define PRINTER_H
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"
#include <string>

#define X_CNTS_PER_MM 1000
#define Y_CNTS_PER_MM 800
#define Z_CNTS_PER_MM 75745.7108f

#define X_STAGE_LEN_MM 150
#define Y_STAGE_LEN_MM 500
#define Z_STAGE_LEN_MM 15

#define PRINT_X_SIZE_MM 100
#define PRINT_Y_SIZE_MM 100

#define ROLLER_1_BIT 18
#define ROLLER_2_BIT 21

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

namespace CMD
{

std::string spread_layer(const RecoatSettings &settings);
//std::string spread_layers();
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
std::string set_reference_time();
std::string at_time_samples(int samples);
std::string at_time_milliseconds(int milliseconds);

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

namespace detail
{
std::string axis_string(Axis axis);

int mm2cnts(double mm, Axis axis);
int um2cnts(double um, Axis axis);

std::string to_ASCII_code(char charToConvert);
std::string create_gcmd(const std::string &command, Axis axis, int quantity);

std::string GCmd();
std::string GMotionComplete();
std::string JetDrive();
std::string GSleep();
std::string Message();
}

}

#endif // PRINTER_H
