/****************************************************************************
** Meta object code from reading C++ file 'SensorManager.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "SensorManager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SensorManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SIM__Coin3D__Quarter__SensorManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      37,   36,   36,   36, 0x0a,
      51,   36,   36,   36, 0x0a,
      66,   36,   36,   36, 0x0a,
      86,   36,   36,   36, 0x0a,
     111,  107,   36,   36, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SIM__Coin3D__Quarter__SensorManager[] = {
    "SIM::Coin3D::Quarter::SensorManager\0"
    "\0idleTimeout()\0delayTimeout()\0"
    "timerQueueTimeout()\0sensorQueueChanged()\0"
    "sec\0setTimerEpsilon(double)\0"
};

void SIM::Coin3D::Quarter::SensorManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SensorManager *_t = static_cast<SensorManager *>(_o);
        switch (_id) {
        case 0: _t->idleTimeout(); break;
        case 1: _t->delayTimeout(); break;
        case 2: _t->timerQueueTimeout(); break;
        case 3: _t->sensorQueueChanged(); break;
        case 4: _t->setTimerEpsilon((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SIM::Coin3D::Quarter::SensorManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SIM::Coin3D::Quarter::SensorManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SIM__Coin3D__Quarter__SensorManager,
      qt_meta_data_SIM__Coin3D__Quarter__SensorManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SIM::Coin3D::Quarter::SensorManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SIM::Coin3D::Quarter::SensorManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SIM::Coin3D::Quarter::SensorManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SIM__Coin3D__Quarter__SensorManager))
        return static_cast<void*>(const_cast< SensorManager*>(this));
    return QObject::qt_metacast(_clname);
}

int SIM::Coin3D::Quarter::SensorManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
