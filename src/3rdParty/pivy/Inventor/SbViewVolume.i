%ignore SbViewVolume::projectPointToLine(const SbVec2f & pt, SbLine & line) const;
%ignore SbViewVolume::projectPointToLine(const SbVec2f & pt, SbVec3f & line0, SbVec3f & line1) const;
%ignore SbViewVolume::projectToScreen(const SbVec3f & src, SbVec3f & dst) const;

%feature("autodoc", "projectPointToLine(SbVec3f) -> (SbVec3f, SbVec3f)") SbViewVolume::projectPointToLine;
%feature("autodoc", "projectToScreen(SbVec3f) -> SbVec3f") SbViewVolume::projectToScreen;

%extend SbViewVolume {
  PyObject * projectPointToLine(const SbVec2f & pt) {
    SbVec3f * line0 = new SbVec3f;
    SbVec3f * line1 = new SbVec3f;

    self->projectPointToLine(pt, *line0, *line1);

    return Py_BuildValue("(OO)",
                         SWIG_NewPointerObj((void *)line0, SWIGTYPE_p_SbVec3f, 1),
                         SWIG_NewPointerObj((void *)line1, SWIGTYPE_p_SbVec3f, 1));
  }

  PyObject * projectToScreen(const SbVec3f & src) {
    SbVec3f * dst = new SbVec3f;

    self->projectToScreen(src, *dst);

    return SWIG_NewPointerObj((void *)dst, SWIGTYPE_p_SbVec3f, 1);
  }
}
