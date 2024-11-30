#!/usr/bin/python


TREE_TYPES = [(dim, "int", "unsigned long long", "i", "L") for dim in range(2,7)] + \
             [(dim, "float", "unsigned long long", "f", "L") for dim in range(2,7)]    


def write_swig_file(tmpl_fn_name, swig_fn_name):
    TMPL_SEPARATOR_DEF="""\
////////////////////////////////////////////////////////////////////////////////
// TYPE (%s) -> %s
////////////////////////////////////////////////////////////////////////////////
"""
    TMPL_SEPARATOR=[]

    TMPL_RECORD_DEF="""\
#define RECORD_%i%s%s record_t<%i, %s, %s> // cf. py-kdtree.hpp
"""
    TMPL_RECORD=[]

    TMPL_IN_CONV_RECORD_DEF="""\
%%typemap(in) RECORD_%i%s%s (RECORD_%i%s%s temp) {
  if (PyTuple_Check($input)) {
    if (PyArg_ParseTuple($input,"(%s)%s", %s, &temp.data)!=0) 
    {
      $1 = temp;
    } else {
      PyErr_SetString(PyExc_TypeError,"tuple must have %i elements: (%i dim %s vector, %s value)");
      return NULL;
    }

  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  } 
 }
"""
    TMPL_IN_CONV_RECORD=[]

    TMPL_IN_CONV_POINT_DEF="""\
%%typemap(in) RECORD_%i%s%s::point_t (RECORD_%i%s%s::point_t point) {
   if (PyTuple_Check($input)) {
     if (PyArg_ParseTuple($input,"%s", %s)!=0) 
     {
       $1 = point;
     } else {
       PyErr_SetString(PyExc_TypeError,"tuple must contain %i ints");
       return NULL;
     }

   } else {
     PyErr_SetString(PyExc_TypeError,"expected a tuple.");
     return NULL;
   } 
  }
 """
    TMPL_IN_CONV_POINT=[]


    TMPL_OUT_CONV_POINT_DEF="""\
 %%typemap(out) RECORD_%i%s%s * {
   RECORD_%i%s%s * r = $1;
   PyObject* py_result;

   if (r != NULL) {

     py_result = PyTuple_New(2);
     if (py_result==NULL) {
       PyErr_SetString(PyErr_Occurred(),"unable to create a tuple.");
       return NULL;
     }

     if (PyTuple_SetItem(py_result, 0, Py_BuildValue("(%s)", %s))==-1) {
       PyErr_SetString(PyErr_Occurred(),"(a) when setting element");

       Py_DECREF(py_result);
       return NULL;
     }

     if (PyTuple_SetItem(py_result, 1, Py_BuildValue("%s", r->data))==-1) {
       PyErr_SetString(PyErr_Occurred(),"(b) when setting element");

       Py_DECREF(py_result);
       return NULL;
     }
   } else {
     py_result = Py_BuildValue("");
   }

   $result = py_result;
  }
 """
    TMPL_OUT_CONV_POINT=[]

    TMPL_OUT_CONV_RECORD_DEF="""\
%%typemap(out) std::vector<RECORD_%i%s%s  >*  {
  std::vector<RECORD_%i%s%s >* v = $1;

  PyObject* py_result = PyList_New(v->size());
  if (py_result==NULL) {
    PyErr_SetString(PyErr_Occurred(),"unable to create a list.");
    return NULL;
  }
  std::vector<RECORD_%i%s%s  >::const_iterator iter = v->begin();

  for (size_t i=0; i<v->size(); i++, iter++) {
    if (PyList_SetItem(py_result, i, Py_BuildValue("(%s)%s", %s, (*iter).data))==-1) {
      PyErr_SetString(PyErr_Occurred(),"(c) when setting element");

      Py_DECREF(py_result);
      return NULL;
    } else {
      //std::cout << "successfully set element " << *iter << std::endl;
    }
  }

  $result = py_result;
 }
"""
    TMPL_OUT_CONV_RECORD=[]

    TMPL_PY_CLASS_DEF="""\
%%template () RECORD_%i%s%s;
%%template (KDTree_%i%s)   PyKDTree<%i, %s, %s>;
"""
    TMPL_PY_CLASS=[]


    TYPE_DEFS = []

    for t in TREE_TYPES:
        dim, coord_t, data_t, py_coord_t, py_data_t = t
        coord_t_short = "".join([_[0] for _ in coord_t.split(" ")])
        data_t_short = "".join([_[0] for _ in data_t.split(" ")])

        TMPL_SEPARATOR.append(TMPL_SEPARATOR_DEF%(",".join([coord_t for _ in range(dim)]), data_t))

        TMPL_RECORD.append(TMPL_RECORD_DEF%(dim, coord_t_short, data_t_short, dim, coord_t, data_t))

        TMPL_IN_CONV_RECORD.append(TMPL_IN_CONV_RECORD_DEF%\
                                   (dim, coord_t_short, data_t_short,
                                    dim, coord_t_short, data_t_short,
                                    py_coord_t*dim, py_data_t, ",".join(["&temp.point[%i]"%i for i in range(dim)]),
                                    dim, dim, coord_t, data_t)
                                   )

        TMPL_IN_CONV_POINT.append(TMPL_IN_CONV_POINT_DEF%\
                                  (dim, coord_t_short, data_t_short,
                                   dim, coord_t_short, data_t_short,
                                   py_coord_t*dim, ",".join(["&point[%i]"%i for i in range(dim)]),
                                   dim)
                                  )

        TMPL_OUT_CONV_RECORD.append(TMPL_OUT_CONV_RECORD_DEF%\
                                    (dim, coord_t_short, data_t_short,
                                     dim, coord_t_short, data_t_short,
                                     dim, coord_t_short, data_t_short,
                                     py_coord_t*dim, py_data_t, ",".join(["(*iter).point[%i]"%i for i in range(dim)]),
                                     )
                                    )
        TMPL_OUT_CONV_POINT.append(TMPL_OUT_CONV_POINT_DEF%\
                                   (dim, coord_t_short, data_t_short,
                                    dim, coord_t_short, data_t_short,
                                    py_coord_t*dim, ",".join(["r->point[%i]"%i for i in range(dim)]),
                                    py_data_t)
                                   )

        TMPL_PY_CLASS.append(TMPL_PY_CLASS_DEF%\
                             (dim, coord_t_short, data_t_short,
                              dim, coord_t.capitalize(), dim, coord_t, data_t)
                             )


    TMPL_BODY_LIST = []
    for i in range(len(TREE_TYPES)):
        TMPL_BODY_LIST.append(TMPL_SEPARATOR[i] + "\n" + \
                              TMPL_RECORD[i] + "\n" + \
                              TMPL_IN_CONV_POINT[i] + "\n" + \
                              TMPL_IN_CONV_RECORD[i] + "\n" + \
                              TMPL_OUT_CONV_POINT[i] + "\n" + \
                              TMPL_OUT_CONV_RECORD[i])

    TMPL_BODY = "\n\n".join(TMPL_BODY_LIST)

    # write swig file
    i_content = open(tmpl_fn_name, "r").read()
    i_content = i_content.replace("%%TMPL_BODY%%", TMPL_BODY).replace("%%TMPL_PY_CLASS_DEF%%", "\n".join(TMPL_PY_CLASS))
    f=open(swig_fn_name, "w")
    f.write(i_content)
    f.close()


def write_hpp_file(tmpl_fn_name, hpp_fn_name):
    TMPL_SEPARATOR_DEF="""\
////////////////////////////////////////////////////////////////////////////////
// Definition of (%s) with data type %s
////////////////////////////////////////////////////////////////////////////////
"""
    TMPL_SEPARATOR=[]

    TMPL_RECORD_DEF = """\
#define RECORD_%i%s%s record_t<%i, %s, %s>
#define KDTREE_TYPE_%i%s%s KDTree::KDTree<%i, RECORD_%i%s%s, std::pointer_to_binary_function<RECORD_%i%s%s,int,double> >
"""
    TMPL_RECORD=[]

    TMPL_OP_EQ_DEF = """\
inline bool operator==(RECORD_%i%s%s const& A, RECORD_%i%s%s const& B) {
    return %s && A.data == B.data;
}
"""
    TMPL_OP_EQ = []

    TMPL_OP_OUT_DEF="""\
std::ostream& operator<<(std::ostream& out, RECORD_%i%s%s const& T)
{
    return out << '(' << %s << '|' << T.data << ')';
}
"""
    TMPL_OP_OUT = []
    
    TYPE_DEFS = []

    for t in TREE_TYPES:
        dim, coord_t, data_t, py_coord_t, py_data_t = t
        coord_t_short = "".join([_[0] for _ in coord_t.split(" ")])
        data_t_short = "".join([_[0] for _ in data_t.split(" ")])

        TMPL_SEPARATOR.append(TMPL_SEPARATOR_DEF%(",".join([coord_t for _ in range(dim)]), data_t))

        TMPL_RECORD.append(TMPL_RECORD_DEF%(dim, coord_t_short, data_t_short,
                                            dim, coord_t, data_t,
                                            dim, coord_t_short, data_t_short,
                                            dim,
                                            dim, coord_t_short, data_t_short,
                                            dim, coord_t_short, data_t_short)
                           )

        TMPL_OP_EQ.append(TMPL_OP_EQ_DEF%(dim, coord_t_short, data_t_short,
                                          dim, coord_t_short, data_t_short,
                                          " && ".join(["A.point[%i] == B.point[%i]"%(i,i) for i in range(dim)])))

        TMPL_OP_OUT.append(TMPL_OP_OUT_DEF%(dim, coord_t_short, data_t_short,
                                            " << ',' << ".join(["T.point[%i]"%i for i in range(dim)])))


    TMPL_BODY_LIST = []
    for i in range(len(TREE_TYPES)):
        TMPL_BODY_LIST.append(TMPL_SEPARATOR[i] + "\n" +  TMPL_RECORD[i] + "\n" + TMPL_OP_EQ[i] + "\n" +  TMPL_OP_OUT[i])

    TMPL_BODY = "\n\n".join(TMPL_BODY_LIST)

    # write hpp file
    hpp_content = open(tmpl_fn_name, "r").read()
    hpp_content = hpp_content.replace("%%TMPL_HPP_DEFS%%", TMPL_BODY)
    f=open(hpp_fn_name, "w")
    f.write(hpp_content)
    f.close()


if __name__=="__main__":
    write_swig_file("py-kdtree.i.tmpl", "py-kdtree.i")
    write_hpp_file("py-kdtree.hpp.tmpl", "py-kdtree.hpp")
    
