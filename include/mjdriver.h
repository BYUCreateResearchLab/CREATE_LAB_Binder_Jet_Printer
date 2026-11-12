#ifndef MJDRIVER_H
#define MJDRIVER_H

#include <QMutex>
#include <functional>
#include "asyncserialdevice.h"
#include <nlohmann/json.hpp>

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

    // Maintenance Commands
    void fill_head(int headIdx);
    void clear_heads();
    void fill_nozzle_range(int headIdx, int start, int span);

    void report_status();
    void set_absolute_start(int steps);
    void set_printing_frequency(int frequency);
    void set_head_voltage(HeadIndex idx, double voltage);
    void mode_select(Mode mode);
    void soft_reset_board();
    void report_current_position();
    void report_head_temps();
    QByteArray convert_image(int headIdx, const QImage &image, int whiteSpace);
    // Uploads the image for one head and waits for the board to acknowledge it.
    // Returns false if the data could not be sent.
    bool send_image_data(int headIdx, const QImage &image, int whiteSpace);

    // --- Waiting on the board (these run the event loop while they wait) ---
    // Call expect_reply() BEFORE sending a command, then wait_for_expected()
    // after it. Returns true once a reply line containing the text arrives.
    void expect_reply(const QString &text);
    bool wait_for_expected(int timeout_ms);
    // Waits until every queued command has actually been sent.
    bool wait_until_idle(int timeout_ms);
    void reconstructed_bitmap(int headIdx, const QByteArray &imageData, int width, int height); // 12/1 added headIdx

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

signals:
    void statusReceived(const nlohmann::json &j);
    void response(QString data);
    void error(QString message);

protected:

    // consider making these virtual functions in the asyncserialdevice ??
    void handle_ready_read();
    void handle_timeout();

    void clear_members();
    void handle_serial_error(QSerialPort::SerialPortError serialPortError);

private:
    void handle_text_line(const QString &line);
    void handle_json(const QByteArray &jsonText);
    bool wait_until(const std::function<bool()> &condition, int timeout_ms);

    QByteArray jsonBuffer;      // a JSON status report that is still arriving
    int jsonDepth {0};          // open '{' minus '}' seen so far in jsonBuffer
    QString expectedReply;      // text set by expect_reply()
    bool expectedReplySeen {false};

    // how long to wait for the board to acknowledge an image upload
    const int imageAckTimeout_ms {3000};
};

}

#endif // MJDRIVER_H
