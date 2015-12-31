/****************************************************************************
** Meta object code from reading C++ file 'ContextMenu.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "ContextMenu.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ContextMenu.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SIM__Coin3D__Quarter__ContextMenu[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      42,   35,   34,   34, 0x0a,
      69,   35,   34,   34, 0x0a,
      96,   35,   34,   34, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SIM__Coin3D__Quarter__ContextMenu[] = {
    "SIM::Coin3D::Quarter::ContextMenu\0\0"
    "action\0changeRenderMode(QAction*)\0"
    "changeStereoMode(QAction*)\0"
    "changeTransparencyType(QAction*)\0"
};

void SIM::Coin3D::Quarter::ContextMenu::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ContextMenu *_t = static_cast<ContextMenu *>(_o);
        switch (_id) {
        case 0: _t->changeRenderMode((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 1: _t->changeStereoMode((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 2: _t->changeTransparencyType((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SIM::Coin3D::Quarter::ContextMenu::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SIM::Coin3D::Quarter::ContextMenu::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SIM__Coin3D__Quarter__ContextMenu,
      qt_meta_data_SIM__Coin3D__Quarter__ContextMenu, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SIM::Coin3D::Quarter::ContextMenu::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SIM::Coin3D::Quarter::ContextMenu::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SIM::Coin3D::Quarter::ContextMenu::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SIM__Coin3D__Quarter__ContextMenu))
        return static_cast<void*>(const_cast< ContextMenu*>(this));
    return QObject::qt_metacast(_clname);
}

int SIM::Coin3D::Quarter::ContextMenu::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
