#ifndef BEDMICROSCOPEWIDGET_H
#define BEDMICROSCOPEWIDGET_H

#include <QWidget>
#include "printerwidget.h"
#include <QTimer>

class MicroscopeWorker;

namespace Ui {
class BedMicroscopeWidget;
}

class BedMicroscopeWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit BedMicroscopeWidget(Printer *printer, QWidget *parent = nullptr);
    ~BedMicroscopeWidget();
    void allow_widget_input(bool allowed) override;
    void connect_to_camera();
    void update_display(const QImage &image);
    void show_microscope_image();
    void set_save_folder();
    void capture_images();
    void export_image(const QString& position);

private:
    Ui::BedMicroscopeWidget *ui;
    QTimer *timer {nullptr};
    QString saveFolderPath;
    MicroscopeWorker *worker {nullptr};
    QThread *workerThread {nullptr};
};

#endif // BEDMICROSCOPEWIDGET_H
