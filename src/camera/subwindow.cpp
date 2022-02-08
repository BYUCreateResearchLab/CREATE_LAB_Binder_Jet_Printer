#include "subwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMainWindow>
#include <QMdiArea>
#include <QMessageBox>
#include <ctime>
#include <tuple>
#include <utility>
#include <QMenuBar>
#include "queyeimage.h"

class SubWindow::InitCameraThread : public QThread
{
private:
    QSharedPointer<Camera> m_Camera;
    UEYE_CAMERA_INFO camera_info;
    int retval{};
public:
    InitCameraThread(QSharedPointer<Camera> Camera, UEYE_CAMERA_INFO info) :
        m_Camera(std::move(Camera)),
        camera_info(info) {}

    void run() override
    {
        retval = m_Camera->Open(camera_info, true);
    }
    int getRetval() const {
        return retval;
    }
};


SubWindow::SubWindow(const UEYE_CAMERA_INFO &info, QWidget *parent) :
    QMdiSubWindow(parent)
{
    qRegisterMetaType<UEYEIMAGEINFO>("UEYEIMAGEINFO");
    setWindowIcon(QIcon(":/new/prefix1/images/ueye_logo.png"));

    m_pDisplayWidget = new Display(this);
    setWidget(m_pDisplayWidget);
    setAttribute(Qt::WA_DeleteOnClose);

    m_camera = QSharedPointer<Camera>(new Camera);
    m_pInitCamThread = new InitCameraThread(m_camera, info);
    connect(m_pInitCamThread, &InitCameraThread::finished, this, &SubWindow::onCameraOpenFinished);
    m_pInitCamThread->start();

    m_dialog = new QProgressDialog(parent);
    m_dialog->setLabelText(tr("Opening camera %1 [%2] ...").arg(QString(info.Model), QString::number(info.dwDeviceID)));
    m_dialog->setCancelButton(nullptr);
    m_dialog->setRange(0, 0);
    m_dialog->setAutoClose(false);
    m_dialog->setMinimumDuration(0);
    m_dialog->setWindowModality(Qt::ApplicationModal);
    m_dialog->setMinimumWidth(300);
    m_dialog->show();
}

QSharedPointer<Camera> SubWindow::camera()
{
    return m_camera;
}

void SubWindow::onCameraOpenFinished()
{
    int cameraOpenRetval = m_pInitCamThread->getRetval();
    delete m_pInitCamThread;

    if(m_dialog)
    {
        m_dialog->close();
        m_dialog->deleteLater();
        m_dialog = nullptr;
    }

    if (cameraOpenRetval == IS_SUCCESS)
    {
        setWindowTitle(m_camera->WindowTitle());

        const auto colorMode{qvariant_cast<SENSORINFO>(m_camera->sensorInfo()).nColorMode};

        int colormode = 0;
        if (IS_COLORMODE_BAYER == colorMode)
        {
            colormode = IS_CM_RGB8_PACKED;
        }
        else if (IS_COLORMODE_MONOCHROME == colorMode)
        {
            colormode = IS_CM_MONO8;
        }
        else
        {
            colormode = m_camera->colorMode().toInt();
        }

        if (colormode != 0 && m_camera->colorMode.setValue(colormode) != IS_SUCCESS)
        {
            QMessageBox::information(this, tr("Error!"), tr("SetColorMode failed"), 0);
        }

        /* get some special camera properties */
        ZeroMemory (&m_camera->m_CameraProps, sizeof(m_camera->m_CameraProps));

        // If the camera does not support a continuous AOI -> it uses special image formats
        m_camera->m_CameraProps.bUsesImageFormats = false;
        INT nAOISupported = 0;
        if (is_ImageFormat(m_camera->getCameraHandle(), IMGFRMT_CMD_GET_ARBITRARY_AOI_SUPPORTED, reinterpret_cast<void*>(&nAOISupported),
                           sizeof(nAOISupported)) == IS_SUCCESS)
        {
            m_camera->m_CameraProps.bUsesImageFormats = (nAOISupported == 0);
        }

        /* set the default image format, if used */
        if (m_camera->m_CameraProps.bUsesImageFormats)
        {
            // search the default formats
            m_camera->m_CameraProps.nImgFmtNormal  = m_camera->searchDefImageFormats(CAPTMODE_FREERUN | CAPTMODE_SINGLE);
            m_camera->m_CameraProps.nImgFmtDefaultNormal = m_camera->m_CameraProps.nImgFmtNormal;
            m_camera->m_CameraProps.nImgFmtTrigger = m_camera->searchDefImageFormats(CAPTMODE_TRIGGER_SOFT_SINGLE);
            m_camera->m_CameraProps.nImgFmtDefaultTrigger = m_camera->m_CameraProps.nImgFmtTrigger;
            // set the default formats
            if ((is_ImageFormat(m_camera->getCameraHandle(), IMGFRMT_CMD_SET_FORMAT, reinterpret_cast<void*>(&m_camera->m_CameraProps.nImgFmtNormal),
                                sizeof(m_camera->m_CameraProps.nImgFmtNormal))) == IS_SUCCESS)
            {
                //m_nImageFormat = nFormat;
                //bRet = TRUE;
            }
        }

        /* setup the capture parameter */
        m_camera->SetupCapture();
        connect(m_camera.data(), static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived), this, &SubWindow::onFrameReceived, Qt::DirectConnection);
        connect(this, &SubWindow::updateDisplay, this, &SubWindow::onUpdateDisplay, Qt::DirectConnection);
        connect(this, &SubWindow::updateImageInfo, this, &SubWindow::onUpdateImageInfo);

        cameraOpenFinished();
        setVisible(true);
    }
    else
    {
        QMessageBox::warning(this, "Camera open", QString("Could not open camera, Error code: %1").arg(cameraOpenRetval));
        close();
    }
}

void SubWindow::onResetCamera()
{
    m_camera->resetToDefaults();
}

Display* SubWindow::display()
{
    return m_pDisplayWidget;
}

void SubWindow::onFrameReceived(ImageBufferPtr buffer)
{
    if (!m_pDisplayWidget->isDisplayOff())
    {
        uEyeAssist::QuEyeImage image(reinterpret_cast<uchar*>(buffer->data()),
            buffer->buffer_props().width,
            buffer->buffer_props().height,
            buffer->buffer_props().colorformat);

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        if (buffer->buffer_props().colorformat == IS_CM_BGR8_PACKED)
        {
            image = std::move(image).rgbSwapped();
        }
#endif

        emit updateDisplay(image);
    }

    emit updateImageInfo(buffer->image_info());
}

void SubWindow::onUpdateDisplay(const QImage& image)
{
    m_pDisplayWidget->setImage(image);
}

void SubWindow::closeEvent(QCloseEvent *closeEvent)
{
    m_camera->Close();

    QMdiSubWindow::closeEvent(closeEvent);
}

void SubWindow::onParameterLoadFromEeprom()
{
    m_camera->readParameterSet();
}

void SubWindow::onParameterLoadFromFile()
{
    auto* fd = new QFileDialog (this, tr("Load camera parameter file"));
    fd->setAcceptMode(QFileDialog::AcceptOpen);

    if(m_camera->isPEAK())
    {
        fd->setNameFilters(QStringList() << tr("Camera Settings (*.cset)") << tr("Ini Files (*.ini)"));
    }
    else
    {
        fd->setNameFilter(tr("Ini Files (*.ini)"));
    }

    fd->setDirectory(userDirectory());

    QString fileName ="";

    if (fd->exec () == QDialog::Accepted)
        fileName = fd->selectedFiles().at(0);

    if (!fileName.isEmpty())
    {
        m_camera->readParameterSet(fileName);
    }
}

void SubWindow::onParameterSaveToEeprom()
{
    m_camera->saveParameterSet();
}

void SubWindow::onParameterSaveToFile()
{
    QString filehint;
    if(m_camera->isPEAK())
    {
        filehint = QString("%1_conf.cset").arg(QString(m_camera->Model()));
    }
    else
    {
        filehint = QString("%1_conf.ini").arg(QString(m_camera->Model()));
    }


    auto* fd = new QFileDialog (this, tr("Save camera parameter file"), QString(), filehint);
    fd->setAcceptMode(QFileDialog::AcceptSave);

    if(m_camera->isPEAK())
    {
        fd->setNameFilters(QStringList() << tr("Camera Settings (*.cset)") << tr("Ini Files (*.ini)"));
        fd->setDefaultSuffix("cset");
    }
    else
    {
        fd->setNameFilter(tr("Ini Files (*.ini)"));
        fd->setDefaultSuffix("ini");
    }


    fd->setDirectory(userDirectory());

    QString fileName ="";

    if (fd->exec () == QDialog::Accepted)
        fileName = fd->selectedFiles().at(0);


    if (!fileName.isEmpty ())
    {
        m_camera->saveParameterSet(fileName);
    }
}

void SubWindow::onCaptureFreerun(bool checked)
{
    if (checked)
    {
        if (m_camera->isLive())
        {
            m_camera->stopVideo();
        }
        m_camera->captureVideo(false);
    }
    else
    {
        m_camera->stopVideo();
    }
}

void SubWindow::onSnapshotFreerun(bool checked)
{
    Q_UNUSED(checked)

    m_camera->freezeVideo(false);
}

void SubWindow::onCaptureTriggered(bool checked)
{
    if (checked)
    {
        if (m_camera->isLive())
        {
            m_camera->stopVideo();
        }
        m_camera->captureVideo(true);
    }
    else
    {
        m_camera->stopVideo();
    }
}

void SubWindow::onSnapshotTriggered(bool checked)
{
    Q_UNUSED(checked)
    m_camera->freezeVideo(true);
    //TODO QMessageBox::information (this, tr("Error!"), QString(tr("FreezeVideo failed (Code: %1)")).arg(ret), 0);
}

void SubWindow::onImageLoad()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), userPictureDirectory(),
                                                    tr("Images (*.png *.bmp *.jpg *.jpeg)"));

    if (!fileName.isEmpty())
    {
        int nRet = m_camera->loadImage(fileName);
        if (nRet != IS_SUCCESS)
        {
            char *pErr = nullptr;
            is_GetError(m_camera->getCameraHandle(), &nRet, &pErr);
            QString strErr(pErr);
            QMessageBox::information(this, tr("Error!"), QString(tr("Loading the image failed with error code: %1\n")).arg(nRet) + strErr);
            qDebug() << "Error: Mainview --> onLoadImage() --> is_LoadImage() : " << nRet;
        }
    }
}

void SubWindow::onImageSave()
{
    auto* fd = new QFileDialog (this, tr("Save the current image"), userPictureDirectory(), "image.bmp");
    fd->setAcceptMode(QFileDialog::AcceptSave);

    QStringList namefilters;
    namefilters << tr("Images (*.bmp *.jpg *.jpeg *png)");
    namefilters << tr("Bitmaps (*.bmp)");
    namefilters << tr("JPEG (*.jpg *.jpeg)");
    namefilters << tr("PNG (*.png)");

    std::function<QString(const QString&)> getDefaultSuffixFromFilter = [=](const QString& selectedFilter){
        if (selectedFilter == namefilters[2])
        {
            return "jpg";
        }
        else if (selectedFilter == namefilters[3])
        {
            return "png";
        }
        return "bmp";
    };
    fd->setNameFilters(namefilters);
    QString fileName;
    if (fd->exec () == QDialog::Accepted)
    {
        fileName = fd->selectedFiles().at(0);
    }
    QFileInfo file_info(fileName);

    QString suffix = file_info.suffix();
    if (suffix.isEmpty())
    {
        suffix = getDefaultSuffixFromFilter(fd->selectedNameFilter());
        fileName += "." + suffix;
    }

    if (!fileName.isEmpty ())
    {
        static const std::map<QString, int> suffix_map{ {"bmp", IS_IMG_BMP }, {"png", IS_IMG_PNG}, {"jpg", IS_IMG_JPG}, {"jpeg", IS_IMG_JPG}};

        auto it = suffix_map.find(suffix);
        if (it != suffix_map.end())
        {
            // set Bitmap params as default
            int fileformat = it->second;
            int fileparam = 0; // !only for IS_IMG_JPEG

            IMAGE_FILE_PARAMS ImageFileParams;
            auto filePath = fileName.toStdWString();

            ImageFileParams.pwchFileName = const_cast<wchar_t*>(filePath.c_str());
            ImageFileParams.pnImageID = nullptr;
            ImageFileParams.ppcImageMem = nullptr;
            ImageFileParams.nQuality = static_cast<UINT>(fileparam);
            ImageFileParams.nFileType = static_cast<UINT>(fileformat);

            int ret = is_ImageFile(m_camera->getCameraHandle(), IS_IMAGE_FILE_CMD_SAVE, reinterpret_cast<void*>(&ImageFileParams), sizeof(ImageFileParams));
            if (ret != IS_SUCCESS)
            {
                QMessageBox::critical(this, tr("Error!"), QString(tr("Saving image failed with code %1!")).arg(ret), QMessageBox::Ok, 0);
            }
        }
        else
        {
            QMessageBox::warning(this, tr("Unkown file format"), tr("The specified format is not supported!"));
            return;
        }
    }
}

void SubWindow::setMarkHotpixel(bool checked)
{
    std::vector<Hotpixel> hotpixels;
    if (checked)
    {
        INT nNumber = 0;
        INT nRet = is_HotPixel(m_camera->getCameraHandle(), IS_HOTPIXEL_GET_MERGED_CAMERA_LIST_NUMBER, reinterpret_cast<void*>(&nNumber), sizeof(nNumber));
        if (nRet == IS_SUCCESS)
        {
            uint32_t u32Size = nNumber * 2 + 1;
            std::unique_ptr<char16_t[]> tmpData(new char16_t[u32Size]);

            nRet = is_HotPixel(
                m_camera->getCameraHandle(), IS_HOTPIXEL_GET_MERGED_CAMERA_LIST, tmpData.get(), u32Size * sizeof(WORD));
            if (nRet == IS_SUCCESS)
            {
                for (int i = 1; i < nNumber; i += 2)
                {
                    hotpixels.push_back({tmpData[i], tmpData[i +1], HotpixelType::Software});
                }
            }
        }
    }

    m_pDisplayWidget->showHotpixel(hotpixels);
}

void SubWindow::setFocusAOI(AUTOFOCUS_AOI aoi, bool visible)
{
    m_pDisplayWidget->setFocusAOI(aoi, visible);
}

void SubWindow::onUpdateImageInfo(const UEYEIMAGEINFO& info)
{
    std::vector<std::tuple<QString,QVariant>> values;

    values.emplace_back("ID", static_cast<qlonglong>(info.u64FrameNumber));
    values.emplace_back("Size", QString::number(info.dwImageWidth) + "x" + QString::number(info.dwImageHeight));
    values.emplace_back("Timestamp", static_cast<qlonglong>(info.u64TimestampDevice));
    values.emplace_back("Time", fromuEyeTime(info.TimestampSystem).toString("yyyy-MM-dd hh:mm:ss"));

    QString html = QStringLiteral("<table style=\"background: rgba(200, 200, 200, 0.6);\" cellpadding=2>\n");

    for(const auto& value: values)
    {
        html += QStringLiteral("<tr><td>") +std::get<0>(value) + QStringLiteral("</td><td>") + std::get<1>(value).toString() + QStringLiteral("</td></tr>\n");
    }

    html += QStringLiteral("</table>");

    m_pDisplayWidget->setInfoText(html);
}

void SubWindow::onImageSaveWithAnnotations()
{
    auto* fd = new QFileDialog (this, tr("Save the current image"),  userPictureDirectory(), "image.bmp");
    fd->setAcceptMode(QFileDialog::AcceptSave);

    QStringList namefilters;
    namefilters << tr("Images (*.bmp *.jpg *.jpeg *png)");
    namefilters << tr("Bitmaps (*.bmp)");
    namefilters << tr("JPEG (*.jpg *.jpeg)");
    namefilters << tr("PNG (*.png)");

    std::function<QString(const QString&)> getDefaultSuffixFromFilter = [=](const QString& selectedFilter){
        if (selectedFilter == namefilters[2])
        {
            return "jpg";
        }
        else if (selectedFilter == namefilters[3])
        {
            return "png";
        }
        return "bmp";
    };

    fd->setNameFilters(namefilters);
    QString fileName;
    if (fd->exec () == QDialog::Accepted)
    {
        fileName = fd->selectedFiles().at(0);
    }

    if (!fileName.isEmpty ())
    {
        QFileInfo file_info(fileName);
        QString suffix = file_info.suffix();
        if (suffix.isEmpty())
        {
            suffix = getDefaultSuffixFromFilter(fd->selectedNameFilter());
            fileName += "." + suffix;
        }

        bool infoTextVisible = display()->isInfoTextVisible();

        if (!infoTextVisible)
        {
            display()->showInfotext(true);
        }

        display()->saveCurrentFrame(fileName);

        if (!infoTextVisible)
        {
            display()->showInfotext(false);
        }
    }
}

void SubWindow::resizeEvent(QResizeEvent* resizeEvent)
{
    fixMaximizedMenu();

    QMdiSubWindow::resizeEvent(resizeEvent);
}

void SubWindow::enterEvent(QEvent* event)
{
    fixMaximizedMenu();

    QMdiSubWindow::enterEvent(event);
}

void SubWindow::fixMaximizedMenu()
{
    if(isMaximized())
    {
        auto* mainWindow = qobject_cast<QMainWindow *>(window());
        if(mainWindow)
        {
            auto* menu = qobject_cast<QMenuBar *>(mainWindow->menuWidget());
            if(menu)
            {
                auto* widget = menu->cornerWidget(Qt::TopLeftCorner);
                if(widget)
                {
                    widget->hide();
                    menu->setCornerWidget(nullptr, Qt::TopLeftCorner);
                }
            }
        }
    }
}
