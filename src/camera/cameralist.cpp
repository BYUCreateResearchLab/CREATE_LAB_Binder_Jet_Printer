#include "cameralist.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QDebug>


CameraList::CameraList(QWidget *parent) :
    QDialog(parent),
    camID(0),
    camCount(0),
    cameraList(nullptr, std::free)
{
    initWidgets();
    initConnections();
    addCameras();

    table->selectRow(0);
    this->setMinimumSize(500, 200);

    list_update = new EventThread(this);
    list_update->start(0, {IS_SET_EVENT_REMOVAL, IS_SET_EVENT_NEW_DEVICE, IS_SET_EVENT_STATUS_CHANGED});

    connect(list_update, &EventThread::eventhappen, this, &CameraList::onUpdateList);
}

void CameraList::onUpdateList(int)
{
    table->reset();
    addCameras();
}

void CameraList::initWidgets()
{
    table = new QTableWidget();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setColumnCount(5);
    QStringList header;
    header << tr("Avaiable") << tr("ID") << tr("Dev. ID") << tr("Model") << tr("Ser. Nr");
    table->setHorizontalHeaderLabels(header);
    table->setSortingEnabled(true);
    table->setSelectionBehavior(QTableView::SelectRows);
    table->setSelectionMode(QTableView::ExtendedSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setDefaultSectionSize(table->verticalHeader()->minimumSectionSize());
    table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setSortIndicatorShown(false);

    auto *layout = new QVBoxLayout();
    layout->addWidget(table);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Close);

    layout->addWidget(buttonBox);
    setLayout(layout);
}

void CameraList::initConnections()
{
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
    connect(table, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
}

int CameraList::cameraID() const
{
    return camID;
}

int CameraList::cameraCount() const
{
    return camCount;
}

std::vector<UEYE_CAMERA_INFO> CameraList::cameraInfo()
{
    return camInfo;
}

void CameraList::addCameras()
{
    is_GetNumberOfCameras(&camCount);
    table->setRowCount(camCount);
    if(camCount >= 1)
    {
        cameraList = std::unique_ptr<UEYE_CAMERA_LIST, decltype(std::free)*>{
            static_cast<UEYE_CAMERA_LIST*>(std::malloc(sizeof(DWORD) + camCount * sizeof(UEYE_CAMERA_INFO))), std::free };

        cameraList->dwCount = camCount;
        is_GetCameraList(cameraList.get());

        for(int i = 0; i < camCount; ++i)
        {
            if(!cameraList->uci[i].dwInUse)
            {
                table->setItem(i, 0, new QTableWidgetItem(QIcon(":/new/prefix1/images/openplay.png"), tr("Yes")));
            }
            else
            {
                table->setItem(i, 0, new QTableWidgetItem(QIcon(":/new/prefix1/images/camclose.png"), tr("No")));
            }

            table->model()->setData(table->model()->index(i, 1), (int)cameraList->uci[i].dwCameraID);
            table->model()->setData(table->model()->index(i, 2), (int)cameraList->uci[i].dwDeviceID);
            table->model()->setData(table->model()->index(i, 3), cameraList->uci[i].Model);
            table->model()->setData(table->model()->index(i, 4), cameraList->uci[i].SerNo);
        }

        table->resizeColumnsToContents();
    }

    buttonBox->buttons().first()->setEnabled(camCount != 0);
}

void CameraList::reject()
{
    list_update->stop();

    QDialog::reject();
}

void CameraList::onSelectionChanged()
{
    auto selected_rows = table->selectionModel()->selectedRows();
    buttonBox->buttons().first()->setEnabled(!selected_rows.empty());
}

void CameraList::accept()
{
    list_update->stop();

    auto selected_rows = table->selectionModel()->selectedRows();
    for (auto& index : selected_rows)
    {
        camInfo.push_back(cameraList->uci[index.row()]);
    }

    QDialog::accept();
}

void CameraList::selectAll() const
{
    table->selectAll();
}

bool CameraList::isSingleCamOpenable()
{
    if(camCount != 1)
    {
        return false;
    }

    if(cameraList->uci[0].dwInUse)
    {
        return false;
    }
    else
    {
        return true;
    }
}
