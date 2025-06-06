#ifndef MJDRIVER_H
#define MJDRIVER_H

#include <QMutex>
#include "asyncserialdevice.h"

namespace Added_Scientific
{

// class for controlling serial communications with
// the multi-nozzle printhead from added scientific
class Controller final : public AsyncSerialDevice
{
    Q_OBJECT

public:

    enum HeadIndex
    {
        HEAD1 = 1,
        HEAD2 = 2,
        HEAD3 = 3,
        HEAD4 = 4
    };
    Q_ENUM(HeadIndex)

    enum Mode
    {
        NONE = 0,
        DROP_WATCHING_INTERNAL = 1,
        DROP_WATCHING_EXTERNAL = 2,
        MOTION_STEPPER = 3,
        MOTION_ENCODER = 4,
        HARDWARE_PRODUCT_DETECT
    };
    Q_ENUM(Mode)

    explicit Controller(const QString &portName, QObject *parent = nullptr);
    ~Controller();
    void connect_board();
    void disconnect_serial();

    void power_on();
    void power_off();

    void report_status();
    void set_absolute_start(double steps);
    void set_printing_frequency(int frequency);
    void set_head_voltage(HeadIndex idx, double voltage);
    void mode_select(Mode mode);
    void soft_reset_board();
    void report_current_position();
    void report_head_temps();
    QByteArray convert_image(int headIdx, const QImage &image, int whiteSpace);
    void send_image_data(int headIdx, const QImage &image, int whiteSpace);
    void reconstructed_bitmap(const QByteArray &imageData, int width, int height);

    void create_bitmap_lines(int numLines, int width);
    void createBitmapTestLines(int numberOfLines,  int lineSpacing, int dropletSpacing, int frequency, int lineLength, int number);
    void createBitmapSet(int numberOfLines,int lineSpacing,int dropletSpacing,int frequency,int lineLength, int frequencyChange, int dropletSpacingChange);
    void outputMessage(QString message);

    void request_status_of_all_heads();

    void clear_all_heads_of_data();
    void enable_all_nozzles();
    void clear_nozzles();
    void external_dropwatch_mode();


    void write_line(const QByteArray &data); // this should really be protected, but is public for testing

protected:

    // consider making these virtual functions in the asyncserialdevice ??
    void handle_ready_read();
    void handle_timeout();

    void clear_members();
    void handle_serial_error(QSerialPort::SerialPortError serialPortError);
};

}

#endif // MJDRIVER_H
