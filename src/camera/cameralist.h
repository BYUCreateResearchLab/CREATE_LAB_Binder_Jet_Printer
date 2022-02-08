/*!
 * \file    cameralist.h
 * \date    26.06.2009
 * \author  IDS Imaging Development Systems GmbH
 * \version 1.0
 * \version $Revision:$
 *
 * \brief   camera list widget declaration
 *
 * \par Last modified
 *      on $Date:$ by $Author:$
 *
 * \par Revision history
 * \li  26.06.2009 - created
 */
#ifndef CAMERALIST_H
#define CAMERALIST_H

#include "eventthread.h"
#include <QDialog>
#include <QMainWindow>
#include <QtGlobal>
#include "utils.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <memory>
#include <ueye.h>

class CameraList : public QDialog
{
    Q_OBJECT

public:
    explicit CameraList(QWidget *parent = nullptr);

    void accept() override;
    void reject() override;

    NO_DISCARD int cameraID() const;
    NO_DISCARD int cameraCount() const;
    NO_DISCARD std::vector<UEYE_CAMERA_INFO> cameraInfo();

    void selectAll() const;
    NO_DISCARD bool isSingleCamOpenable();

private:
    void initWidgets();
    void initConnections();
    void addCameras();

    QTableWidget *table{};
    QDialogButtonBox *buttonBox{};

    int camID;
    int camCount;
    std::vector<UEYE_CAMERA_INFO> camInfo;
    std::unique_ptr<UEYE_CAMERA_LIST, decltype(std::free)*> cameraList;

    EventThread *list_update;

public slots:
    void onUpdateList(int event);
    void onSelectionChanged();

};
/*!
 * \}
 */// end of doc group CAMERALIST

#endif // CAMERALIST_H
