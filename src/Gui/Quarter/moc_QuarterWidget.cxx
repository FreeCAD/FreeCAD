/****************************************************************************
** Meta object code from reading C++ file 'QuarterWidget.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "QuarterWidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QuarterWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SIM__Coin3D__Quarter__QuarterWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
      11,   44, // properties
       3,   77, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      37,   36,   36,   36, 0x0a,
      47,   36,   36,   36, 0x0a,
      54,   36,   36,   36, 0x0a,
      68,   63,   36,   36, 0x0a,
      94,   63,   36,   36, 0x0a,
     125,  120,   36,   36, 0x0a,

 // properties: name, type, flags
     168,  163, 0x11095107,
     194,  187, 0x43095103,
     215,  210, 0x01095103,
     234,  210, 0x01095103,
     251,  210, 0x01095103,
     264,  210, 0x01095103,
     276,  210, 0x01095103,
     299,  210, 0x01095103,
     334,  317, 0x0009510b,
     362,  351, 0x0009510b,
     384,  373, 0x0009510b,

 // enums: name, flags, count, data
     317, 0x0,   11,   89,
     351, 0x0,    6,  111,
     373, 0x0,    5,  123,

 // enum data: key, value
     395, uint(SIM::Coin3D::Quarter::QuarterWidget::SCREEN_DOOR),
     407, uint(SIM::Coin3D::Quarter::QuarterWidget::ADD),
     411, uint(SIM::Coin3D::Quarter::QuarterWidget::DELAYED_ADD),
     423, uint(SIM::Coin3D::Quarter::QuarterWidget::SORTED_OBJECT_ADD),
     441, uint(SIM::Coin3D::Quarter::QuarterWidget::BLEND),
     447, uint(SIM::Coin3D::Quarter::QuarterWidget::DELAYED_BLEND),
     461, uint(SIM::Coin3D::Quarter::QuarterWidget::SORTED_OBJECT_BLEND),
     481, uint(SIM::Coin3D::Quarter::QuarterWidget::SORTED_OBJECT_SORTED_TRIANGLE_ADD),
     515, uint(SIM::Coin3D::Quarter::QuarterWidget::SORTED_OBJECT_SORTED_TRIANGLE_BLEND),
     551, uint(SIM::Coin3D::Quarter::QuarterWidget::NONE),
     556, uint(SIM::Coin3D::Quarter::QuarterWidget::SORTED_LAYERS_BLEND),
     576, uint(SIM::Coin3D::Quarter::QuarterWidget::AS_IS),
     582, uint(SIM::Coin3D::Quarter::QuarterWidget::WIREFRAME),
     592, uint(SIM::Coin3D::Quarter::QuarterWidget::WIREFRAME_OVERLAY),
     610, uint(SIM::Coin3D::Quarter::QuarterWidget::POINTS),
     617, uint(SIM::Coin3D::Quarter::QuarterWidget::HIDDEN_LINE),
     629, uint(SIM::Coin3D::Quarter::QuarterWidget::BOUNDING_BOX),
     642, uint(SIM::Coin3D::Quarter::QuarterWidget::MONO),
     647, uint(SIM::Coin3D::Quarter::QuarterWidget::ANAGLYPH),
     656, uint(SIM::Coin3D::Quarter::QuarterWidget::QUAD_BUFFER),
     668, uint(SIM::Coin3D::Quarter::QuarterWidget::INTERLEAVED_ROWS),
     685, uint(SIM::Coin3D::Quarter::QuarterWidget::INTERLEAVED_COLUMNS),

       0        // eod
};

static const char qt_meta_stringdata_SIM__Coin3D__Quarter__QuarterWidget[] = {
    "SIM::Coin3D::Quarter::QuarterWidget\0"
    "\0viewAll()\0seek()\0redraw()\0mode\0"
    "setRenderMode(RenderMode)\0"
    "setStereoMode(StereoMode)\0type\0"
    "setTransparencyType(TransparencyType)\0"
    "QUrl\0navigationModeFile\0QColor\0"
    "backgroundColor\0bool\0contextMenuEnabled\0"
    "headlightEnabled\0clearZBuffer\0clearWindow\0"
    "interactionModeEnabled\0interactionModeOn\0"
    "TransparencyType\0transparencyType\0"
    "RenderMode\0renderMode\0StereoMode\0"
    "stereoMode\0SCREEN_DOOR\0ADD\0DELAYED_ADD\0"
    "SORTED_OBJECT_ADD\0BLEND\0DELAYED_BLEND\0"
    "SORTED_OBJECT_BLEND\0"
    "SORTED_OBJECT_SORTED_TRIANGLE_ADD\0"
    "SORTED_OBJECT_SORTED_TRIANGLE_BLEND\0"
    "NONE\0SORTED_LAYERS_BLEND\0AS_IS\0WIREFRAME\0"
    "WIREFRAME_OVERLAY\0POINTS\0HIDDEN_LINE\0"
    "BOUNDING_BOX\0MONO\0ANAGLYPH\0QUAD_BUFFER\0"
    "INTERLEAVED_ROWS\0INTERLEAVED_COLUMNS\0"
};

void SIM::Coin3D::Quarter::QuarterWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QuarterWidget *_t = static_cast<QuarterWidget *>(_o);
        switch (_id) {
        case 0: _t->viewAll(); break;
        case 1: _t->seek(); break;
        case 2: _t->redraw(); break;
        case 3: _t->setRenderMode((*reinterpret_cast< RenderMode(*)>(_a[1]))); break;
        case 4: _t->setStereoMode((*reinterpret_cast< StereoMode(*)>(_a[1]))); break;
        case 5: _t->setTransparencyType((*reinterpret_cast< TransparencyType(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SIM::Coin3D::Quarter::QuarterWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SIM::Coin3D::Quarter::QuarterWidget::staticMetaObject = {
    { &QGraphicsView::staticMetaObject, qt_meta_stringdata_SIM__Coin3D__Quarter__QuarterWidget,
      qt_meta_data_SIM__Coin3D__Quarter__QuarterWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SIM::Coin3D::Quarter::QuarterWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SIM::Coin3D::Quarter::QuarterWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SIM::Coin3D::Quarter::QuarterWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SIM__Coin3D__Quarter__QuarterWidget))
        return static_cast<void*>(const_cast< QuarterWidget*>(this));
    return QGraphicsView::qt_metacast(_clname);
}

int SIM::Coin3D::Quarter::QuarterWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QUrl*>(_v) = navigationModeFile(); break;
        case 1: *reinterpret_cast< QColor*>(_v) = backgroundColor(); break;
        case 2: *reinterpret_cast< bool*>(_v) = contextMenuEnabled(); break;
        case 3: *reinterpret_cast< bool*>(_v) = headlightEnabled(); break;
        case 4: *reinterpret_cast< bool*>(_v) = clearZBuffer(); break;
        case 5: *reinterpret_cast< bool*>(_v) = clearWindow(); break;
        case 6: *reinterpret_cast< bool*>(_v) = interactionModeEnabled(); break;
        case 7: *reinterpret_cast< bool*>(_v) = interactionModeOn(); break;
        case 8: *reinterpret_cast< TransparencyType*>(_v) = transparencyType(); break;
        case 9: *reinterpret_cast< RenderMode*>(_v) = renderMode(); break;
        case 10: *reinterpret_cast< StereoMode*>(_v) = stereoMode(); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: setNavigationModeFile(*reinterpret_cast< QUrl*>(_v)); break;
        case 1: setBackgroundColor(*reinterpret_cast< QColor*>(_v)); break;
        case 2: setContextMenuEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 3: setHeadlightEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 4: setClearZBuffer(*reinterpret_cast< bool*>(_v)); break;
        case 5: setClearWindow(*reinterpret_cast< bool*>(_v)); break;
        case 6: setInteractionModeEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 7: setInteractionModeOn(*reinterpret_cast< bool*>(_v)); break;
        case 8: setTransparencyType(*reinterpret_cast< TransparencyType*>(_v)); break;
        case 9: setRenderMode(*reinterpret_cast< RenderMode*>(_v)); break;
        case 10: setStereoMode(*reinterpret_cast< StereoMode*>(_v)); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::ResetProperty) {
        switch (_id) {
        case 0: resetNavigationModeFile(); break;
        }
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 11;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 11;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_END_MOC_NAMESPACE
