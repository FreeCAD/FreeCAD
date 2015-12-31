/****************************************************************************
** Meta object code from reading C++ file 'iistaskheader.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "iistaskheader.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'iistaskheader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_iisTaskHeader[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x05,

 // slots: signature, parameters, type, tag, flags
      27,   14,   14,   14, 0x0a,
      34,   14,   14,   14, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_iisTaskHeader[] = {
    "iisTaskHeader\0\0activated()\0fold()\0"
    "animate()\0"
};

void iisTaskHeader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        iisTaskHeader *_t = static_cast<iisTaskHeader *>(_o);
        switch (_id) {
        case 0: _t->activated(); break;
        case 1: _t->fold(); break;
        case 2: _t->animate(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData iisTaskHeader::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject iisTaskHeader::staticMetaObject = {
    { &QFrame::staticMetaObject, qt_meta_stringdata_iisTaskHeader,
      qt_meta_data_iisTaskHeader, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &iisTaskHeader::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *iisTaskHeader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *iisTaskHeader::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_iisTaskHeader))
        return static_cast<void*>(const_cast< iisTaskHeader*>(this));
    return QFrame::qt_metacast(_clname);
}

int iisTaskHeader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void iisTaskHeader::activated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
