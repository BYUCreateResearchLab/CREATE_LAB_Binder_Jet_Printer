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
    // look at DataRecievedHandler in "MJ Driver Board/Software/DriverBoardDropwatcher/Form1.cs"
    while (serialPort->bytesAvailable())
    {
        auto inData = serialPort->readAll();
        readData.append(inData);
    }

    if (readData[0] == '{')
    {
        json j;
        try
        {
            j = json::parse(readData.toStdString());
            // handle the json here
            emit response( QString::fromStdString(j.dump(4)) ); // this probably doesn't need to be here and clogs the output window

        }
        catch (const nlohmann::json::parse_error &e)
        {
            QString errorMessage = "Failed to parse JSON: ";
            errorMessage += e.what();
            emit error(errorMessage);
        }

    }

    else
    {
        emit response(QString(readData));
    }

    readData.clear();
    write_next();
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

void Controller::send_image_data(int headIdx, const QImage &image, int whiteSpace)
{
    QByteArray imageData = convert_image(headIdx, image, whiteSpace);



    write(imageData);
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



} // namespace Added_Scientific

#include "moc_mjdriver.cpp"
