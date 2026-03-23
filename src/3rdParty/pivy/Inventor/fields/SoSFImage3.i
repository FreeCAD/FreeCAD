%typemap(in,numinputs=0) (SbVec3s & size, int & nc) (int temp) {
  $1 = new SbVec3s();
  $2 = &temp;
}

%typemap(argout) (SbVec3s & size, int & nc) {
  Py_XDECREF($result); /* free up any previous result */
  $result = Py_BuildValue("(s#Oi)",
                          (const char *)result,
                          (*$1)[0] * (*$1)[1] * (*$1)[2] * (*$2),
                          SWIG_NewPointerObj((void *)$1, SWIGTYPE_p_SbVec3s, 1),
                          *$2);
}

%extend SoSFImage3 {
  void setValue(const SbVec3s & size, const int nc, PyObject * pixels)
  {
    Py_ssize_t len = size[0] * size[1] * size[2] * nc;
    unsigned char * image;
#ifdef PY_2
    PyString_AsStringAndSize(pixels, (char **)&image, &len);
#else    
    PyObject *  b_pixels = pixels;
    if  (PyUnicode_Check(pixels)){
      b_pixels = PyUnicode_AsEncodedString(pixels, "utf-8", "Error ~");
    }
    PyBytes_AsStringAndSize(b_pixels, (char **)&image, &len);
#endif
    self->setValue(size, nc, image);
  }
  void setValue(const SoSFImage3 * other) { *self = *other; }
}
