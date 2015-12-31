/****************************************************************************
** Meta object code from reading C++ file 'actiongroup.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "actiongroup.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'actiongroup.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_QSint__ActionGroup[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       3,   54, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x0a,
      42,   31,   19,   19, 0x0a,
      62,   19,   19,   19, 0x2a,
      85,   78,   19,   19, 0x0a,
     101,   19,   19,   19, 0x2a,
     119,  113,   19,   19, 0x0a,
     142,   19,   19,   19, 0x09,
     156,   19,   19,   19, 0x09,

 // properties: name, type, flags
      31,  170, 0x01095103,
     175,  170, 0x01095103,
     190,  182, 0x0a095103,

       0        // eod
};

static const char qt_meta_stringdata_QSint__ActionGroup[] = {
    "QSint::ActionGroup\0\0showHide()\0"
    "expandable\0setExpandable(bool)\0"
    "setExpandable()\0enable\0setHeader(bool)\0"
    "setHeader()\0title\0setHeaderText(QString)\0"
    "processHide()\0processShow()\0bool\0"
    "header\0QString\0headerText\0"
};

void QSint::ActionGroup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ActionGroup *_t = static_cast<ActionGroup *>(_o);
        switch (_id) {
        case 0: _t->showHide(); break;
        case 1: _t->setExpandable((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->setExpandable(); break;
        case 3: _t->setHeader((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->setHeader(); break;
        case 5: _t->setHeaderText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->processHide(); break;
        case 7: _t->processShow(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData QSint::ActionGroup::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject QSint::ActionGroup::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QSint__ActionGroup,
      qt_meta_data_QSint__ActionGroup, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QSint::ActionGroup::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QSint::ActionGroup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QSint::ActionGroup::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QSint__ActionGroup))
        return static_cast<void*>(const_cast< ActionGroup*>(this));
    return QWidget::qt_metacast(_clname);
}

int QSint::ActionGroup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = isExpandable(); break;
        case 1: *reinterpret_cast< bool*>(_v) = hasHeader(); break;
        case 2: *reinterpret_cast< QString*>(_v) = headerText(); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setExpandable(*reinterpret_cast< bool*>(_v)); break;
        case 1: setHeader(*reinterpret_cast< bool*>(_v)); break;
        case 2: setHeaderText(*reinterpret_cast< QString*>(_v)); break;
        }
        _id -= 3;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
