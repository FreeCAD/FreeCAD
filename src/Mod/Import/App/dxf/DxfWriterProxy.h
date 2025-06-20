#ifndef DXFWRITERPROXY_H
#define DXFWRITERPROXY_H

#include <Python.h>
#include "dxf/ImpExpDxf.h"

namespace Import
{

typedef struct
{
    PyObject_HEAD ImpExpDxfWrite* writer_inst;
} DxfWriterProxy;

extern PyTypeObject DxfWriterProxy_Type;

}  // namespace Import

#endif  // DXFWRITERPROXY_H
