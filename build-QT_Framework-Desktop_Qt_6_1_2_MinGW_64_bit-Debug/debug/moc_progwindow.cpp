/****************************************************************************
** Meta object code from reading C++ file 'progwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.1.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../QT_Framework/progwindow.h"
#include <QtGui/qtextcursor.h>
#include <QScreen>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'progwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.1.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_progWindow_t {
    const uint offsetsAndSize[44];
    char stringdata0[327];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_progWindow_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_progWindow_t qt_meta_stringdata_progWindow = {
    {
QT_MOC_LITERAL(0, 10), // "progWindow"
QT_MOC_LITERAL(11, 11), // "firstWindow"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 20), // "on_back2Home_clicked"
QT_MOC_LITERAL(45, 23), // "on_numSets_valueChanged"
QT_MOC_LITERAL(69, 4), // "arg1"
QT_MOC_LITERAL(74, 26), // "on_tableWidget_cellChanged"
QT_MOC_LITERAL(101, 3), // "row"
QT_MOC_LITERAL(105, 6), // "column"
QT_MOC_LITERAL(112, 22), // "on_startX_valueChanged"
QT_MOC_LITERAL(135, 22), // "on_startY_valueChanged"
QT_MOC_LITERAL(158, 26), // "on_setSpacing_valueChanged"
QT_MOC_LITERAL(185, 33), // "on_printPercentSlider_sliderM..."
QT_MOC_LITERAL(219, 8), // "position"
QT_MOC_LITERAL(228, 23), // "on_clearConsole_clicked"
QT_MOC_LITERAL(252, 21), // "on_startPrint_clicked"
QT_MOC_LITERAL(274, 12), // "printLineSet"
QT_MOC_LITERAL(287, 6), // "setNum"
QT_MOC_LITERAL(294, 1), // "e"
QT_MOC_LITERAL(296, 7), // "GReturn"
QT_MOC_LITERAL(304, 2), // "rc"
QT_MOC_LITERAL(307, 19) // "connectToController"

    },
    "progWindow\0firstWindow\0\0on_back2Home_clicked\0"
    "on_numSets_valueChanged\0arg1\0"
    "on_tableWidget_cellChanged\0row\0column\0"
    "on_startX_valueChanged\0on_startY_valueChanged\0"
    "on_setSpacing_valueChanged\0"
    "on_printPercentSlider_sliderMoved\0"
    "position\0on_clearConsole_clicked\0"
    "on_startPrint_clicked\0printLineSet\0"
    "setNum\0e\0GReturn\0rc\0connectToController"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_progWindow[] = {

 // content:
       9,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   92,    2, 0x06,    0 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    0,   93,    2, 0x08,    1 /* Private */,
       4,    1,   94,    2, 0x08,    2 /* Private */,
       6,    2,   97,    2, 0x08,    4 /* Private */,
       9,    1,  102,    2, 0x08,    7 /* Private */,
      10,    1,  105,    2, 0x08,    9 /* Private */,
      11,    1,  108,    2, 0x08,   11 /* Private */,
      12,    1,  111,    2, 0x08,   13 /* Private */,
      14,    0,  114,    2, 0x08,   15 /* Private */,
      15,    0,  115,    2, 0x08,   16 /* Private */,
      16,    1,  116,    2, 0x08,   17 /* Private */,
      18,    1,  119,    2, 0x08,   19 /* Private */,
      21,    0,  122,    2, 0x08,   21 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    7,    8,
    QMetaType::Void, QMetaType::Double,    5,
    QMetaType::Void, QMetaType::Double,    5,
    QMetaType::Void, QMetaType::Double,    5,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,

       0        // eod
};

void progWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<progWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->firstWindow(); break;
        case 1: _t->on_back2Home_clicked(); break;
        case 2: _t->on_numSets_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->on_tableWidget_cellChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->on_startX_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->on_startY_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 6: _t->on_setSpacing_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 7: _t->on_printPercentSlider_sliderMoved((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->on_clearConsole_clicked(); break;
        case 9: _t->on_startPrint_clicked(); break;
        case 10: _t->printLineSet((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->e((*reinterpret_cast< GReturn(*)>(_a[1]))); break;
        case 12: _t->connectToController(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (progWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&progWindow::firstWindow)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject progWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_progWindow.offsetsAndSize,
    qt_meta_data_progWindow,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_progWindow_t
, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<GReturn, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *progWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *progWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_progWindow.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int progWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void progWindow::firstWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
