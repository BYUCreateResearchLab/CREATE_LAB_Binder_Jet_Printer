#include "dropletobservationwidget.h"
#include "ui_dropletobservationwidget.h"

#include "ueye.h"

#include "camera.h"
#include "display.h"
#include <QDialog>
#include <QMainWindow>
#include <QtGlobal>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QRadioButton>
#include <QGroupBox>
#include <QSize>
#include <QPoint>
#include <QLayout>
#include <QThread>
#include <QMutex>
#include <QUrl>
#include <QCloseEvent>
#include <QTimerEvent>
#include <QWidget>
#include <QProgressBar>
#include <QTimer>
#include <QTime>
#include <QTranslator>
#include <QActionGroup>
#include <QMenu>
#include <QStatusBar>

#include <QDebug>

DropletObservationWidget::DropletObservationWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::DropletObservationWidget)
{
    ui->setupUi(this);
    connect(ui->connect, &QPushButton::clicked, this, &DropletObservationWidget::connect_to_camera);
}

DropletObservationWidget::~DropletObservationWidget()
{
    delete ui;
}

void DropletObservationWidget::allow_widget_input(bool allowed)
{

}

void DropletObservationWidget::connect_to_camera()
{
    int numCameras{-1}; 
    is_GetNumberOfCameras(&numCameras); // get the number of connected cameras
    //UEYE_CAMERA_LIST cameraList;
    //cameraList.dwCount = numCameras; // tell the list how many cameras are connected
    qDebug() << QString("There are %1 cameras currently connected").arg(numCameras);

    HIDS hCam = 0; //Open the first available camera
    int nRet = is_InitCamera(&hCam, NULL);

    if (nRet != IS_SUCCESS)
    {
        //Check if GigE uEye SE needs a new starter firmware
        if (nRet == IS_STARTER_FW_UPLOAD_NEEDED)
        {
            //Calculate time needed for updating the starter firmware
            int nTime;
            is_GetDuration (hCam, IS_SE_STARTER_FW_UPLOAD, &nTime);
            /*
       e.g. have progress bar displayed in separate thread
      */

            //Upload new starter firmware during initialization
            hCam = hCam | IS_ALLOW_STARTER_FW_UPLOAD;
            nRet = is_InitCamera(&hCam, NULL);

            /*
        end progress bar
       */
        }
    }
    /*
    // get the commandline start ID
    auto camList = GetCameraList();
    auto info = camList[0];
    bool live{true};


    auto *subWindow = new SubWindow(info);
    auto mdiCount = ui->mdiArea->subWindowList().size();
    int numCams = 1;

    // huge lambda expression to run when the cameraOpenFinished signal is sent
    connect(subWindow, &SubWindow::cameraOpenFinished, this, [this, live, subWindow, numCams, mdiCount]() {
        bool maximized = true;

        ui->mdiArea->addSubWindow(subWindow);

        if (live)
        {
            subWindow->camera()->captureVideo(false);
        }

        if (numCams == 1)
        {
            subWindow->showMaximized();
        }
        else if(mdiCount == 0 || maximized)
        {
            subWindow->showMaximized();
        }

    });

    /*
    if (actionProperties->isChecked())
    {
        connect(m_pPropertiesDlg, &Properties::markHotpixel, subWindow, &SubWindow::setMarkHotpixel);
        connect(m_pPropertiesDlg, &Properties::focusAOIChanged, subWindow, &SubWindow::setFocusAOI);
    }
    */
}


