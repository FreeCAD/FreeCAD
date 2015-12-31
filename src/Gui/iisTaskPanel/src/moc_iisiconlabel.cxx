/****************************************************************************
** Meta object code from reading C++ file 'iisiconlabel.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "iisiconlabel.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'iisiconlabel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_iisIconLabel[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x05,
      24,   13,   13,   13, 0x05,
      35,   13,   13,   13, 0x05,
      45,   13,   13,   13, 0x05,
      57,   13,   13,   13, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_iisIconLabel[] = {
    "iisIconLabel\0\0pressed()\0released()\0"
    "clicked()\0activated()\0contextMenu()\0"
};

void iisIconLabel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        iisIconLabel *_t = static_cast<iisIconLabel *>(_o);
        switch (_id) {
        case 0: _t->pressed(); break;
        case 1: _t->released(); break;
        case 2: _t->clicked(); break;
        case 3: _t->activated(); break;
        case 4: _t->contextMenu(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData iisIconLabel::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject iisIconLabel::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_iisIconLabel,
      qt_meta_data_iisIconLabel, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &iisIconLabel::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *iisIconLabel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *iisIconLabel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_iisIconLabel))
        return static_cast<void*>(const_cast< iisIconLabel*>(this));
    return QWidget::qt_metacast(_clname);
}

int iisIconLabel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void iisIconLabel::pressed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void iisIconLabel::released()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void iisIconLabel::clicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void iisIconLabel::activated()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void iisIconLabel::contextMenu()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
