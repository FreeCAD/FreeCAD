%ignore SbPlane::intersect(const SbLine& l, SbVec3f& intersection) const;
%ignore SbPlane::intersect(const SbPlane & pl, SbLine & line) const;

%feature("autodoc", "intersect(SbLine) -> SbVec3f") SbPlane::intersect;
%feature("autodoc", "intersect(SbPlane) -> SbLine") SbPlane::intersect;

%extend SbPlane {
    int __eq__(const SbPlane & u) { return *self == u; }
    int __ne__(const SbPlane & u) { return *self != u; }

    PyObject * intersect(const SbLine& l)
    {
        SbVec3f * point = new SbVec3f;
        self->intersect(l, *point);
        return SWIG_NewPointerObj((void *)point, SWIGTYPE_p_SbVec3f, 1);
    }

    PyObject * intersect(const SbPlane & pl)
    {
        SbLine * line = new SbLine;
        self->intersect(pl, *line);
        return SWIG_NewPointerObj((void *)line, SWIGTYPE_p_SbLine, 1);
    }
}
    
