#include "mjdriver.h"

#include <QSerialPort>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QTime>
#include <QCoreApplication>
#include <QImage>
#include <QBitmap>
#include <QPainter>
#include <format>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

namespace Added_Scientific
{

using json = nlohmann::json;

Controller::Controller(const QString &portName, QObject *parent) :
    AsyncSerialDevice(portName, parent)
{
    name = "MJ_Controller";
    // connect timer for handling timeout errors
    connect(serialPort, &QSerialPort::readyRead, this, &Controller::handle_ready_read);
    connect(timer, &QTimer::timeout, this, &Controller::handle_timeout);
    connect(serialPort, &QSerialPort::errorOccurred, this, &Controller::handle_serial_error);

    serialPort->setBaudRate(1'000'000); // 1 million baud rate.
}

Controller::~Controller()
{
    if (is_connected()) serialPort->close();
}

void Controller::handle_ready_read()
{
    readData.append(serialPort->readAll());

    // The board sends plain text lines (replies to commands) and a JSON
    // status report that spans several lines. Split the data into lines,
    // collect the JSON lines until the braces balance, and emit the rest.
    int newlineIndex;
    while ((newlineIndex = static_cast<int>(readData.indexOf('\n'))) != -1)
    {
        const QByteArray rawLine = readData.left(newlineIndex);
        readData.remove(0, newlineIndex + 1);
        const QByteArray line = rawLine.trimmed();

        if (jsonDepth > 0 || line.startsWith('{'))
        {
            jsonBuffer.append(rawLine).append('\n');
            jsonDepth += static_cast<int>(rawLine.count('{') - rawLine.count('}'));
            if (jsonDepth <= 0)
            {
                handle_json(jsonBuffer);
                jsonBuffer.clear();
                jsonDepth = 0;
            }
            else if (jsonBuffer.size() > 65536)
            {
                // never closed: drop it so it can't block later reports
                emit error(QString("%1: discarded an incomplete status report").arg(name));
                jsonBuffer.clear();
                jsonDepth = 0;
            }
            continue;
        }

        if (!line.isEmpty()) handle_text_line(QString::fromUtf8(line));
    }

    // The closing brace of a report may arrive without a newline after it
    if (jsonDepth > 0 && !readData.isEmpty())
    {
        const int depthWithRest =
            jsonDepth + static_cast<int>(readData.count('{') - readData.count('}'));
        if (depthWithRest <= 0)
        {
            jsonBuffer.append(readData);
            readData.clear();
            handle_json(jsonBuffer);
            jsonBuffer.clear();
            jsonDepth = 0;
        }
    }
}

void Controller::handle_json(const QByteArray &jsonText)
{
    try {
        auto j = nlohmann::json::parse(jsonText.constData());
        if (j.contains("heads")) {
            emit statusReceived(j);
        }
        timer->stop(); // got a response, stop the timeout timer
    } catch (...) {
        emit error(QString("%1: could not read a status report").arg(name));
    }
}

void Controller::handle_text_line(const QString &line)
{
    if (!expectedReply.isEmpty() && line.contains(expectedReply, Qt::CaseInsensitive))
    {
        expectedReplySeen = true;
    }
    emit response(line);
}

void Controller::expect_reply(const QString &text)
{
    expectedReply = text;
    expectedReplySeen = false;
}

bool Controller::wait_for_expected(int timeout_ms)
{
    const bool seen = wait_until([this]{ return expectedReplySeen; }, timeout_ms);
    expectedReply.clear();
    expectedReplySeen = false;
    return seen;
}

bool Controller::wait_until_idle(int timeout_ms)
{
    return wait_until([this]{ return is_write_idle(); }, timeout_ms);
}

// Runs the event loop until the condition is true, the time runs out,
// or the board is disconnected. Serial data is only sent and received
// while the event loop runs, so this must not be replaced with a sleep.
bool Controller::wait_until(const std::function<bool()> &condition, int timeout_ms)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!condition())
    {
        if (!is_connected() || elapsed.elapsed() > timeout_ms) return false;
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    return true;
}

void Controller::handle_timeout()
{
    timer->stop();
    emit error(QString("Serial IO Timeout: No response from %1").arg(name));
    emit error(readData);
    disconnect_serial();
}

// writes to serial device, adding a LF character ('\n')
// the controller expects LF after every command
void Controller::write_line(const QByteArray &data)
{
    write(data + "\n");
}

void Controller::connect_board()
{
    if (is_connected()) // return if already connected
    {
        emit response(QString("Already connected to %1").arg(name));
        return;
    }

    clear_members();

    // attempt to open connection to serial port
    if (!serialPort->open(QIODevice::ReadWrite))
    {
        emit error(QString("Can't open %1 on %2, error code %3")
                   .arg(name, serialPort->portName(), QVariant::fromValue(serialPort->error()).toString()));
        return;
    }

    // connection is open
    emit response(QString("Connected to %1").arg(name));
    return;
}

void Controller::disconnect_serial()
{
    clear_members();
    if (is_connected())
    {
        emit response(QString("Disconnecting %1").arg(name));
        serialPort->close();
        timer->stop();
    }
    else emit response(QString("%1 is already disconnected").arg(name));
}

void Controller::power_on()
{
    write_line("O");
}

void Controller::power_off()
{
    write_line("F");
}

void Controller::report_status()
{
    write_line("b");
}

void Controller::set_printing_frequency(int frequency)
{
    if (frequency <= 0) return;
    QString command = QString("p %1").arg(frequency);
    write_line(command.toUtf8());
}

void Controller::set_head_voltage(HeadIndex idx, double voltage)
{
    // only allow voltage between 15-36 V
    if ((voltage < 15.0) || (voltage > 36.0)) return;
    QString command = "v ";
    command += QString::number(static_cast<int>(idx));
    command += " ";
    command += QString::number(voltage, 'f', 2); // floating with 2 decimals of precision
    write_line(command.toUtf8());
}

void Controller::set_absolute_start(int steps)
{
    if (steps <= 0) return;
    QString command = QString(", %1").arg(steps);
    write_line(command.toUtf8());
}



void Controller::mode_select(Mode mode)
{
    QString command = QString("M %1").arg(static_cast<int>(mode));
    write_line(command.toUtf8());
}

QByteArray Controller::convert_image(int headIdx, const QImage &image, int whiteSpace)
{
    // Convert image to grayscale if needed
    QImage grayimage = image.convertToFormat(QImage::Format_Grayscale8);

    // Get image properties
    int width = grayimage.width();
    int height = grayimage.height();
    emit response(QString("Height = %1, Width = %2").arg(height).arg(width)); // for debugging
    const uchar *pixelData = grayimage.bits();

    // Array to store pixel values
    std::vector<float> processedPixels(width * height);

    // Normalize pixel values between 0 and 1
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = y * width + x;
            processedPixels[index] = (255.0f - pixelData[index]) / 255.0f;
        }
    }

    // Array to store data
    QByteArray imageData;
    imageData.append(87); // Sends 'W' command first
    imageData.append(100 + headIdx);

    // Keep track of sum and count
    long long sumofval = 0;
    long long copnt = 0;
    long long lastval = 0;

    // Pre-image whitespace
    if(whiteSpace != 0)
    {
        for (int i = 0; i < whiteSpace; ++i)
        {
            for (int byt = 0; byt < 16; ++byt)
            {
                unsigned char curByte = 0;
                imageData.append(curByte);
                lastval = curByte;
                sumofval += static_cast<int>(lastval);
                copnt += 1;
            }
        }
    }

    // Convert image to binary string
    for (int i = 0; i < width; ++i)
    {
        for (int byt = 0; byt < 16; ++byt)
        {
            unsigned char curByte = 0;
            for (int bit = 0; bit < 8; ++bit)
            {
                int j = byt*8 + bit;
                // 12/1 added 2nd material logic
                if (j < height) // j < height && processedPixels[j * width + i] > 0.5f previous condition
                {
                    unsigned char pixelColor = pixelData[j * width + i];
                        if (headIdx == 1 && pixelColor < 50) { // black pixels on head 1
                            curByte += 1 << (7 - bit);
                        } else if (headIdx == 2 && pixelColor > 100 && pixelColor < 200) { // grey pixels on head 2
                            curByte += 1 << (7 - bit);
                        }
                }
            }
            imageData.append(curByte);
            lastval = curByte;
            // emit response(QString("curByte = %1").arg(curByte));
            sumofval += static_cast<int>(lastval);
            copnt += 1;
        }
    }

    // Check image is correct by reconstructing and saving it to view
    reconstructed_bitmap(headIdx, imageData, width, height);

    emit response(QString("lastval = %1, sumofval = %2, copnt = %3").arg(lastval).arg(sumofval).arg(copnt));

    return imageData;
}

void Controller::reconstructed_bitmap(int headIdx, const QByteArray &imageData, int width, int height)
{
    // Skip first two bytes
    int startIdx = 2;

    // Create empty bitmap
    QBitmap newBitmap(width, height);
    newBitmap.clear();

    QPainter painter(&newBitmap);
    painter.setPen(Qt::black);

    // Reconstruct image
    for (int i = 0; i < width; ++i)
    {
        for (int byt = 0; byt < 16; ++byt)
        {
            if (startIdx >= imageData.size()) break;
            unsigned char curByte = static_cast<unsigned char>(imageData[startIdx++]);
            for (int bit = 0; bit < 8; ++bit)
            {
                int j = byt*8 + bit;
                if (j < height)
                {
                    bool pixelOn = (curByte & (1 << (7 - bit))) != 0;
                    if (pixelOn) painter.drawPoint(i,j);
                }
            }
        }
    }

    painter.end();

    // Convert to QImage for saving
    QImage image = newBitmap.toImage();

    // save to file library
    QString filePath = QString("C:\\Users\\CB140LAB\\Desktop\\Noah\\debugBitmap%1.bmp").arg(headIdx); // 12/1 saves separate files for head 1/2
    image.save(filePath);

    emit response(QString("Bitmap reconstructed and saved: %1").arg(filePath));
}

bool Controller::send_image_data(int headIdx, const QImage &image, int whiteSpace)
{
    QByteArray imageData = convert_image(headIdx, image, whiteSpace);

    if (!is_connected())
    {
        emit error(QString("Can't send image. %1 is not connected").arg(name));
        return false;
    }

    // Commands queued before this image must reach the board first
    if (!wait_until_idle(2000))
    {
        emit response(QString("WARNING: earlier commands were still waiting when the head %1 image was sent").arg(headIdx));
    }

    // The board ends an upload when the data stops arriving, so nothing
    // else may be written until it has acknowledged this image.
    set_write_hold(true);
    expect_reply("DL:"); // last line of the board's upload report
    serialPort->write(imageData);

    const bool sent = wait_until([this]{ return serialPort->bytesToWrite() == 0; }, 10000);
    const bool acknowledged = wait_for_expected(sent ? imageAckTimeout_ms : 0);
    set_write_hold(false);

    if (!sent)
    {
        emit error(QString("Image for head %1 was not fully sent").arg(headIdx));
        return false;
    }
    if (!acknowledged)
    {
        emit response(QString("WARNING: no upload report from the board for head %1 after %2 ms. Continuing.")
                          .arg(headIdx).arg(imageAckTimeout_ms));
    }
    return true;
}

void Controller::create_bitmap_lines(int numLines, int width)
{
    // Define the height of the bitmap
    const int height = 128;

    // Create an empty QBitmap
    QBitmap bitmap(width, height);
    bitmap.clear(); // Start with a white bitmap

    QPainter painter(&bitmap);
    painter.setPen(Qt::black);

    // Calculate the spacing between black lines
    int spacing = height / (numLines);

    // Draw black lines
    for (int i = 0; i < numLines; ++i) {
        int y = i * spacing;
        painter.drawLine(0, y, width, y);
    }

    painter.end();

    // Convert QBitmap to QImage for saving
    QImage image = bitmap.toImage();

    // Save the image to the specified file path
    QString filePath = "C:\\Users\\CB140LAB\\Desktop\\Noah\\currentBitmap.bmp";
    image.save(filePath);

    emit response(QString("Bitmap created and saved as currentBitmap.bmp"));
}

void Controller::createBitmapTestLines(int numberOfLines,  int lineSpacing, int dropletSpacing, int frequency, int lineLength, int number)
{
    int numNozzles = 128;
    int bedWidth = 100; //mm
    int gap = (numNozzles - (numberOfLines * lineSpacing)) / 2;
    int yCoord = gap;
    int xCoordS = 0;
    double dropSpacing = dropletSpacing;
    int xCoordF = lineLength / (dropSpacing / 1000);


    if(numberOfLines > numNozzles-gap*2 || numberOfLines * lineSpacing > numNozzles-gap*2){
        emit response(QString("Too many lines!"));
            return;
    }

    //speed/frequency = spacing (in the x)
    //  (mm/s) / (1/s) = mm

    double printSpeed = (dropSpacing / 1000) * frequency;    //(uM * Freq) -> MM/S

    // Define the height of the bitmap
    const int height = 128;

    // Create an empty QBitmap
    QBitmap bitmap(xCoordF, height);
    bitmap.clear(); // Start with a white bitmap

    QPainter painter(&bitmap);
    painter.setPen(Qt::black);

        //VISUAL X SPACING                          (1 row)
        for(int i = 0; i < numberOfLines; i++){
            painter.drawLine(xCoordS, yCoord, xCoordF, yCoord);
            yCoord = yCoord + lineSpacing;
        }

    painter.end();

    //QString formattedSpeed = QString::number(printSpeed, 'd', 1);
    QString formattedSpeed = QString("%1").arg(QString::number(printSpeed, 'f', 1), 5, QChar('0'));
    QString formattedNumber = QString("%1").arg(number, 2, 10, QChar('0'));
    QString formattedFrequency = QString("%1").arg(frequency, 4, 10, QChar('0'));

    // Convert QBitmap to QImage for saving
    QImage image = bitmap.toImage();
    QString filePath = QString("C:\\Users\\CB140LAB\\Desktop\\Noah\\BitmapTestFolder\\%1testBitmap_%2_%3.bmp").arg(formattedNumber, formattedSpeed, formattedFrequency);
    image.save(filePath);

    emit response(QString("Bitmap created and saved as %1testBitMap_%2_%3.bmp").arg(formattedNumber, formattedSpeed, formattedFrequency));
}

void Controller::createBitmapSet(int numberOfLines,int lineSpacing,int dropletSpacing,int frequency,int lineLength, int frequencyChange, int dropletSpacingChange){
    //6 rows by 10 columns
    int counter = 0;
    for(int i = 0; i < 6; i++){
        int tempDropletSpacing = dropletSpacing + (dropletSpacingChange * i);
        for(int j = 0; j < 10; j++){
            int tempFrequency = frequency + (frequencyChange * j);
            createBitmapTestLines(numberOfLines, lineSpacing, tempDropletSpacing, tempFrequency, lineLength, counter);
            counter++;
        }
    }
}

void Controller::outputMessage(QString message){
    emit response(message);
}

void Controller::soft_reset_board()
{
    write_line("r");
}

void Controller::report_current_position()
{
    write_line(">");
}

void Controller::report_head_temps()
{
    write_line("t");
}

void Controller::request_status_of_all_heads()
{
    write_line("B");
}

void Controller::enable_all_nozzles()
{
    write_line("I 1");
}

void Controller::clear_nozzles()
{
    write_line("C");
}

void Controller::external_dropwatch_mode()
{
    write_line("M 2");
}

void Controller::clear_all_heads_of_data()
{
    write_line("C");
}

void Controller::clear_members()
{
    clear_command_queue();
}

void Controller::handle_serial_error(
        QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        emit error(QObject::tr("Read error on port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::OpenError)
    {
        emit error(QObject::tr("Error opening port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::WriteError)
    {
        emit error(QObject::tr("Error writing to port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }
}

// --- Maintenance Commands (Lowercase) ---

void Controller::fill_head(int headIdx) {
    // lowercase 'f' for fill + index + newline
    QString cmd = QString("I %1").arg(headIdx);
    write_line(cmd.toUtf8());
}

void Controller::clear_heads() {
    // lowercase 'c' for clear + index + newline
    QString cmd = QString("C");
    write_line(cmd.toUtf8());
}

void Controller::fill_nozzle_range(int headIdx, int start, int span) {
    // Command 'N' + Head + Start + Span
    QString cmd = QString("N %1 %2 %3").arg(headIdx).arg(start).arg(span);
    write_line(cmd.toUtf8());
}


} // namespace Added_Scientific

#include "moc_mjdriver.cpp"
