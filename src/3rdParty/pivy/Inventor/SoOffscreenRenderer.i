%extend SoOffscreenRenderer {
  PyObject * getBuffer() {
    SbVec2s size = self->getViewportRegion().getWindowSize();
#ifdef PY_2
    return PyString_FromStringAndSize((char *)self->getBuffer(),
                                      size[0] * size[1] * self->getComponents());
#else
    return PyBytes_FromStringAndSize((char *)self->getBuffer(),
                                      size[0] * size[1] * self->getComponents());
#endif
  }
}
