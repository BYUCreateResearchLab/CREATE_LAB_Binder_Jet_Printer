/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.1.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../QT_Framework/mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.1.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    const uint offsetsAndSize[32];
    char stringdata0[287];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 10), // "MainWindow"
QT_MOC_LITERAL(11, 20), // "on_yPositive_clicked"
QT_MOC_LITERAL(32, 0), // ""
QT_MOC_LITERAL(33, 20), // "on_xPositive_clicked"
QT_MOC_LITERAL(54, 20), // "on_yNegative_clicked"
QT_MOC_LITERAL(75, 20), // "on_xNegative_clicked"
QT_MOC_LITERAL(96, 16), // "on_xHome_clicked"
QT_MOC_LITERAL(113, 16), // "on_yHome_clicked"
QT_MOC_LITERAL(130, 25), // "on_zStepSize_valueChanged"
QT_MOC_LITERAL(156, 4), // "arg1"
QT_MOC_LITERAL(161, 15), // "on_zMax_clicked"
QT_MOC_LITERAL(177, 14), // "on_zUp_clicked"
QT_MOC_LITERAL(192, 16), // "on_zDown_clicked"
QT_MOC_LITERAL(209, 15), // "on_zMin_clicked"
QT_MOC_LITERAL(225, 30), // "on_activateRoller_stateChanged"
QT_MOC_LITERAL(256, 30) // "on_activateHopper_stateChanged"

    },
    "MainWindow\0on_yPositive_clicked\0\0"
    "on_xPositive_clicked\0on_yNegative_clicked\0"
    "on_xNegative_clicked\0on_xHome_clicked\0"
    "on_yHome_clicked\0on_zStepSize_valueChanged\0"
    "arg1\0on_zMax_clicked\0on_zUp_clicked\0"
    "on_zDown_clicked\0on_zMin_clicked\0"
    "on_activateRoller_stateChanged\0"
    "on_activateHopper_stateChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       9,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   92,    2, 0x08,    0 /* Private */,
       3,    0,   93,    2, 0x08,    1 /* Private */,
       4,    0,   94,    2, 0x08,    2 /* Private */,
       5,    0,   95,    2, 0x08,    3 /* Private */,
       6,    0,   96,    2, 0x08,    4 /* Private */,
       7,    0,   97,    2, 0x08,    5 /* Private */,
       8,    1,   98,    2, 0x08,    6 /* Private */,
      10,    0,  101,    2, 0x08,    8 /* Private */,
      11,    0,  102,    2, 0x08,    9 /* Private */,
      12,    0,  103,    2, 0x08,   10 /* Private */,
      13,    0,  104,    2, 0x08,   11 /* Private */,
      14,    1,  105,    2, 0x08,   12 /* Private */,
      15,    1,  108,    2, 0x08,   14 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Int,    9,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_yPositive_clicked(); break;
        case 1: _t->on_xPositive_clicked(); break;
        case 2: _t->on_yNegative_clicked(); break;
        case 3: _t->on_xNegative_clicked(); break;
        case 4: _t->on_xHome_clicked(); break;
        case 5: _t->on_yHome_clicked(); break;
        case 6: _t->on_zStepSize_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->on_zMax_clicked(); break;
        case 8: _t->on_zUp_clicked(); break;
        case 9: _t->on_zDown_clicked(); break;
        case 10: _t->on_zMin_clicked(); break;
        case 11: _t->on_activateRoller_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->on_activateHopper_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSize,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t

, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
