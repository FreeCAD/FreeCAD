/*
PPEMBED, VERSION 2.0
AN ENHANCED PYTHON EMBEDDED-CALL INTERFACE

Copyright 1996-2000, by Mark Lutz, and O'Reilly and Associates.
Permission to use, copy, modify, and distribute this software 
for any purpose and without fee is hereby granted.  This software
is provided on an as is basis, without warranties of any kind.
*/

#include <FCConfig.h>
#include "PyTools.h"

#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <compile.h>
#include <eval.h>


#if PY_VERSION_HEX <= 0x02050000
#error "Use Python2.5.x or higher"
#endif


/*****************************************************************************
 * RUN EMBEDDED OBJECT METHODS, ACCESS OBJECT ATTRIBUTES 
 * handles attribute fetch, debugging, input/output conversions; 
 * there is no module to reload here: assumes a known object;
 *****************************************************************************/

int
PP_Run_Method(PyObject *pobject,  const char *method,
                  const char *resfmt,   void *cresult,        /* convert to c/c++ */
                  const char *argfmt,   ... /* arg,... */ )   /* convert to python */
{
    PyObject *pmeth, *pargs, *presult;
    va_list argslist;                              /* "pobject.method(args)" */
    va_start(argslist, argfmt);

    Py_Initialize();                               /* init if first time */
    pmeth = PyObject_GetAttrString(pobject, method);  
    if (pmeth == NULL)                             /* get callable object */
        return -1;                                 /* bound method? has self */
	/* handle zero args different */
//	if(resfmt == 0 || strcmp(resfmt,"") == 0)
//		pargs = Py_BuildValue("()");
//	else
		pargs = Py_VaBuildValue(argfmt, argslist);     /* args: c->python */

    if (pargs == NULL) {
        Py_DECREF(pmeth);
        return -1;
    }
    if (PP_DEBUG)                                    /* debug it too? */ 
        presult = PP_Debug_Function(pmeth, pargs); 
    else 
        presult = PyEval_CallObject(pmeth, pargs);   /* run interpreter */

    Py_DECREF(pmeth);
    Py_DECREF(pargs);
//	if(cresult != 0 && resfmt != 0)
		return PP_Convert_Result(presult, resfmt, cresult);    /* to C format */
//	else 
//		return 0;
}
 

int
PP_Get_Member(PyObject *pobject, const char *attrname,
                  const char *resfmt,  void *cresult)         /* convert to c/c++ */
{
    PyObject *pmemb;                                    /* "pobject.attrname" */
    Py_Initialize();                        
    pmemb = PyObject_GetAttrString(pobject, attrname);  /* incref'd */
    return PP_Convert_Result(pmemb, resfmt, cresult);   /* to C form, decrefs */
}
 

int
PP_Set_Member(PyObject *pobject, const char *attrname,
                  const char *argfmt,  ... /* arg,... */ )    /* convert to python */
{
    int result;
    PyObject *pval;
    va_list argslist;                             /* "pobject.attrname = v" */
    va_start(argslist, argfmt);
    Py_Initialize();                              /* init if first time */
    pval = Py_VaBuildValue(argfmt, argslist);     /* input: C->Python */
    if (pval == NULL) 
        return -1;
    result = PyObject_SetAttrString(pobject, attrname, pval);     /* setattr */
    Py_DECREF(pval); 
    return result;
}


/*****************************************************************************
 * RUN EMBEDDED MODULE FUNCTIONS 
 * handles module (re)import, debugging, input/output conversions;  
 * note: also useful for calling classes (and C type constructors) at the 
 * top-level of a module to make Python instances: use class-name (or type
 * constructor function name) and 'O' result convert-code to get raw object;
 * use argfmt="()" for no args, cresult='NULL' for no result (procedure);
 * New tools: support for calling known Python objects directly;
 *****************************************************************************/

int
PP_Run_Function(const char *modname, const char *funcname,          /* load from module */
                const char *resfmt,  void *cresult,           /* convert to c/c++ */
                const char *argfmt,  ... /* arg, arg... */ )  /* convert to python */
{
    /* call a function or class in a module */
    PyObject *func, *args, *presult;
    va_list argslist;
    va_start(argslist, argfmt);                   /* "modname.funcname(args)" */

    func = PP_Load_Attribute(modname, funcname);  /* may reload; incref'd */
    if (func == NULL)                             /* func or class or C type */
        return -1;
    args = Py_VaBuildValue(argfmt, argslist);     /* convert args to python */
    if (args == NULL) {                           /* args incref'd */
        Py_DECREF(func);
        return -1;
    }
    if (PP_DEBUG && strcmp(modname, "pdb") != 0)    /* debug this call? */
        presult = PP_Debug_Function(func, args);    /* run in pdb; incref'd */
    else
        presult = PyEval_CallObject(func, args);    /* run function; incref'd */

    Py_DECREF(func);
    Py_DECREF(args);                                    /* result may be None */
    return PP_Convert_Result(presult, resfmt, cresult); /* convert result to C*/
}


PyObject *
PP_Debug_Function(PyObject *func, PyObject *args)
{
    int oops, res;
    PyObject *presult;

    /* expand tuple at front */
    // it seems that some versions of python want just 2 arguments; in that
    // case, remove trailing 1
#if (PY_MAJOR_VERSION==2)&&(PY_MINOR_VERSION>=2)
    oops = _PyTuple_Resize(&args, (1 + PyTuple_Size(args))); 
#else
    oops = _PyTuple_Resize(&args, (1 + PyTuple_Size(args)),1); 
#endif    
    oops |= PyTuple_SetItem(args, 0, func);   
    if (oops) 
        return NULL;                        /* "args = (funcobj,) + (arg,..)" */

    res = PP_Run_Function(                  /* "pdb.runcall(funcobj, arg,..)" */
                 "pdb",  "runcall",         /* recursive run_function */
                 "O",    &presult,
                 "O",     args);            /* args already is a tuple */
    return (res != 0) ? NULL : presult;     /* errors in run_function? */
}                                           /* presult not yet decref'd */


int
PP_Run_Known_Callable(PyObject *object,               /* func|class|method */
                      const char *resfmt, void *cresult,    /* skip module fetch */
                      const char *argfmt, ... /* arg,.. */) /* convert args, result */
{
    /* call a known callable object */
    PyObject *args, *presult;
    va_list argslist;
    va_start(argslist, argfmt);                     /* "return object(args)" */

    Py_Initialize(); 
    args = Py_VaBuildValue(argfmt, argslist);       /* convert args to python */
    if (args == NULL)                               /* args incref'd */
        return -1;
    if (PP_DEBUG)                                   /* debug this call? */
        presult = PP_Debug_Function(object, args);  /* run in pdb; incref'd */
    else
        presult = PyEval_CallObject(object, args);  /* run function; incref'd */

    Py_DECREF(args);                                    /* result may be None */
    return PP_Convert_Result(presult, resfmt, cresult); /* convert result to C*/
}

/*****************************************************************************
 * PYTHON EXCEPTION INFORMATION ACCESS
 * fetch Python-related error info (type, value);
 * after an API call returns an exception indicator, call 
 * PP_Fetch_Error_Text, then get text from the 3 char[]'s;
 * note: calling PyErr_Fetch() clears/erases the current 
 * exception in the Python system, as does PyErr_Print(), 
 * so you should call one of these, one time, per exception:
 * caveats: not thread-specific since saves data in globals,
 * and only exports traceback object (the exception type and 
 * data are converted to text strings and disgarded);  the 
 * PyErr_Print() built-in also does a bit more on syntax errors,
 * and sends its text to sys.stderr: in principle, we could
 * assign stderr to a StringIO object and call PyErr_Print, but
 * the code here makes the 3 exception components more distinct;
 *****************************************************************************/

#define MAX 2024
/*
FC_OS_LINUX: This is dangerous. How about PY_EXCEPT_MAX?
*/

/* exception text is here after PP_Fetch_Error_Text call */
char PP_last_error_type[MAX];           /* exception name text */
char PP_last_error_info[MAX];           /* exception data text */
char PP_last_error_trace[MAX];          /* exception traceback text */
PyObject *PP_last_traceback = NULL;     /* saved exception traceback object */


void PP_Fetch_Error_Text()
{
    // called without exception happened!
    //assert(PyErr_Occurred());

    char *tempstr;
    PyObject *errobj, *errdata, *errtraceback, *pystring;

    /* get latest python exception information */
    /* this also clears the current exception  */

    PyErr_Fetch(&errobj, &errdata, &errtraceback);       /* all 3 incref'd */


    /* convert type and data to strings */
    /* calls str() on both to stringify */

    pystring = NULL;
    if (errobj != NULL &&
       (pystring = PyObject_Str(errobj)) != NULL &&      /* str(errobj) */
       (PyString_Check(pystring)) )                      /* str() increfs */
    {
        strncpy(PP_last_error_type, PyString_AsString(pystring), MAX); /*Py->C*/
        PP_last_error_type[MAX-1] = '\0';
    }
    else 
        strcpy(PP_last_error_type, "<unknown exception type>");
    Py_XDECREF(pystring);


    pystring = NULL;
    if (errdata != NULL &&
       (pystring = PyObject_Str(errdata)) != NULL &&     /* str(): increfs */
       (PyString_Check(pystring)) )
    {
        strncpy(PP_last_error_info, PyString_AsString(pystring), MAX); /*Py->C*/
        PP_last_error_info[MAX-1] = '\0';
    }
    else 
        strcpy(PP_last_error_info, "<unknown exception data>");
    Py_XDECREF(pystring);


    /* convert traceback to string */ 
    /* print text to a StringIO.StringIO() internal file object, then */
    /* fetch by calling object's .getvalue() method (see lib manual); */

    pystring = NULL;
    if (errtraceback != NULL &&
       (PP_Run_Function("StringIO", "StringIO", "O", &pystring, "()") == 0) &&
       (PyTraceBack_Print(errtraceback, pystring) == 0) &&
       (PP_Run_Method(pystring, "getvalue", "s", &tempstr, "()") == 0) )
    {
        strncpy(PP_last_error_trace, tempstr, MAX); 
        PP_last_error_trace[MAX-1] = '\0';
        free(tempstr);  /* it's a strdup */
    }
    else 
        strcpy(PP_last_error_trace, "<unknown exception traceback>"); 
    Py_XDECREF(pystring);


    Py_XDECREF(errobj);
    Py_XDECREF(errdata);               /* this function owns all 3 objects */
    Py_XDECREF(PP_last_traceback);     /* they've been NULL'd out in Python */ 
    PP_last_traceback = errtraceback;  /* save/export raw traceback object */
}


/*****************************************************************************
 * GET/SET MODULE-LEVEL (GLOBAL) PYTHON VARIABLES BY NAME
 * handles module (re)loading, input/output conversions;
 * useful for passing data to/from codestrings (no args or return 
 * val)--load module, set inputs, run codestring, get outputs;
 * subtle thing: Python "s" output conversion code sets a C char* to 
 * the text in the middle of a Python string object, which may be
 * returned to the heap if decref'd--this api copies the text to a new
 * char array (with strdup) that the caller must free() when possible,
 * rather than assuming the caller can and will copy the result out;
 * could postpone the decref till next api call, but that's too subtle;
 *****************************************************************************/


int
PP_Convert_Result(PyObject *presult, const char *resFormat, void *resTarget)
{
    if (presult == NULL)                /* error when run: fail */
        return -1;
    else
    if (resTarget == NULL) {            /* passed target=NULL: ignore result */
        Py_DECREF(presult);             /* procedures and stmts return None  */
        return 0;
    }
    else
    if (! PyArg_Parse(presult, resFormat, resTarget)) {  /* convert Python->C */
        Py_DECREF(presult);                              /* need not be tuple */
        return -1;                                       /* error in convert  */
    }
    else {
        if (strcmp(resFormat, "O") != 0) {     /* free object unless exported */
            if (strcmp(resFormat, "s") == 0) { /* copy string: caller owns it */
                char **target = (char**) resTarget;
#if defined (__GNUC__)
                *target = strdup(*target);
#else
                *target = _strdup(*target);
#endif
            }
            Py_DECREF(presult);
        }
        return 0;                     /* returns 0=success, -1=failure */
    }                                 /* if 0: C result in *resTarget  */
}                                     /* caller must decref if fmt="O" */
                                      /* caller must free() if fmt="s" */

int
PP_Get_Global(const char *modname, const char *varname, const char *resfmt, void *cresult)
{
    PyObject *var;                                   /* "x = modname.varname" */
    var = PP_Load_Attribute(modname, varname);       /* var is incref'd */
    return PP_Convert_Result(var, resfmt, cresult);  /* convert var to C form */
}


int
PP_Set_Global(const char *modname, const char *varname, const char *valfmt, ... /* cval(s) */) 
{
    int result;
    PyObject *module, *val;                     /* "modname.varname = val" */
    va_list cvals;
    va_start(cvals, valfmt);                    /* C args after valfmt */

    module = PP_Load_Module(modname);           /* get/load module */
    if (module == NULL) 
        return -1;
    val = Py_VaBuildValue(valfmt, cvals);       /* convert input to Python */
    va_end(cvals);
    if (val == NULL) 
        return -1;
    result = PyObject_SetAttrString(module, varname, val); 
    Py_DECREF(val);                             /* set global module var */
    return result;                              /* decref val: var owns it */
}                                               /* 0=success, varname set */


/*****************************************************************************
 * MODULE INTERFACE 
 * make/import/reload a python module by name
 * Note that Make_Dummy_Module could be implemented to keep a table
 * of generated dictionaries to be used as namespaces, rather than 
 * using low level tools to create and mark real modules; this 
 * approach would require extra logic to manage and use the table;
 * see basic example of using dictionaries for string namespaces;
 *****************************************************************************/


int PP_RELOAD = 0;    /* reload modules dynamically? */
int PP_DEBUG  = 0;    /* debug embedded code with pdb? */


const char *PP_Init(const char *modname) {
    Py_Initialize();                               /* init python if needed */
//#ifdef FC_OS_LINUX /* cannot convert `const char *' to `char *' in assignment */
    if (modname!=NULL) return modname;
    { /* we assume here that the caller frees allocated memory */
      char* __main__=(char *)malloc(sizeof("__main__"));
      return __main__="__main__";
    }
//#else    
//    return modname == NULL ? "__main__" : modname;  /* default to '__main__' */
//#endif    
}


int
PP_Make_Dummy_Module(const char *modname)   /* namespace for strings, if no file */
{                                     /* instead of sharing __main__ for all */
    PyObject *module, *dict;          /* note: __main__ is created in py_init */
    Py_Initialize();
    module = PyImport_AddModule(modname);    /* fetch or make, no load */
    if (module == NULL)                      /* module not incref'd */
        return -1;                  
    else {                                            /* module.__dict__ */
        dict = PyModule_GetDict(module);              /* ['__dummy__'] = None */
        PyDict_SetItemString(dict, "__dummy__", Py_None); 
        PyDict_SetItemString(dict, "__builtins__", PyEval_GetBuiltins());
        return 0;
    }
}


PyObject *                          /* returns module object named modname  */
PP_Load_Module(const char *modname)       /* modname can be "package.module" form */
{                                   /* reload just resets C extension mods  */
    /* 
     * 4 cases:
     * - module "__main__" has no file, and not prebuilt: fetch or make
     * - dummy modules have no files: don't try to reload them
     * - reload=on and already loaded (on sys.modules): "reload()" before use
     * - not loaded yet, or loaded but reload=off: "import" to fetch or load 
     */

    PyObject *module, *sysmods;                  
    modname = PP_Init(modname);                       /* default to __main__ */

    if (strcmp(modname, "__main__") == 0)             /* main: no file */
        return PyImport_AddModule(modname);           /* not increfd */

    sysmods = PyImport_GetModuleDict();               /* get sys.modules dict */
    module  = PyDict_GetItemString(sysmods, modname); /* mod in sys.modules? */
    
    if (module != NULL &&                             /* dummy: no file */
        PyModule_Check(module) && 
        PyDict_GetItemString(PyModule_GetDict(module), "__dummy__")) {
        return module;                                /* not increfd */
    }
    else
    if (PP_RELOAD && module != NULL && PyModule_Check(module)) {
        module = PyImport_ReloadModule(module);       /* reload file,run code */
        Py_XDECREF(module);                           /* still on sys.modules */
        return module;                                /* not increfd */
    }
    else {  
        module = PyImport_ImportModule(modname);      /* fetch or load module */
        Py_XDECREF(module);                           /* still on sys.modules */
        return module;                                /* not increfd */
    }
}


PyObject *
PP_Load_Attribute(const char *modname, const char *attrname)
{
    PyObject *module;                         /* fetch "module.attr" */
    modname = PP_Init(modname);               /* use before PyEval_CallObject */
    module  = PP_Load_Module(modname);        /* not incref'd, may reload */
    if (module == NULL)
        return NULL;
    return PyObject_GetAttrString(module, attrname);  /* func, class, var,.. */
}                                                     /* caller must xdecref */


/* extra ops */
int
PP_Run_Command_Line(const char *prompt)
{
    int res;               /* interact with python, in "__main__" */
    Py_Initialize();       /* in the program's "stdio" window     */
    if (prompt != NULL)
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX)
        printf("[%s <Use Ctrl-D (i.e. EOF) to exit.>]\n", prompt);
#elif defined (FC_OS_WIN32)
        printf("[%s <Use Ctrl-Z plus Return to exit.>]\n", prompt);
#endif
    res = PyRun_InteractiveLoop(stdin, "<stdin>");
    return res;
}

/*****************************************************************************
 * RUN EMBEDDED CODE-STRINGS 
 * handles debugging, module (re)loading, namespaces, output conversions;
 * pbd.runeval returns a value: "eval(expr + '\n', globals, locals)";
 * pdb.run is just a statement: "exec cmd + '\n' in globals, locals"
 * New tools: precompiling strings to bytecode, running bytecode; 
 *****************************************************************************/



int
PP_Run_Codestr(PPStringModes mode, const char *code,  /* expr or stmt string */
               const char *modname,                   /* loads module if needed */
               const char *resfmt, void *cresult)     /* converts expr result to C */
{
    /* run a string of Python code */
    int parse_mode;                             /* "eval(code, d, d)", or */
    PyObject *module, *dict, *presult;          /* "exec code in d, d" */

    module = PP_Load_Module(modname);           /* get module, init python */
    if (module == NULL)                         /* not incref'd */
        return -1;
    dict = PyModule_GetDict(module);            /* get dict namespace */
    if (dict == NULL)                           /* not incref'd */
        return -1;

    parse_mode = (mode == PP_EXPRESSION ? Py_eval_input : Py_file_input);
    if (PP_DEBUG) 
        presult = PP_Debug_Codestr(mode, code, dict);         /* run in pdb */
    else 
        presult = PyRun_String(code, parse_mode, dict, dict); /* eval direct */
                                                              /* increfs res */
    if (mode == PP_STATEMENT) {
        int result = (presult == NULL? -1 : 0);          /* stmt: 'None' */
        Py_XDECREF(presult);                             /* ignore result */
        return result;
    }
    return PP_Convert_Result(presult, resfmt, cresult);  /* expr val to C */
}


PyObject *
PP_Compile_Codestr(PPStringModes mode,    /* precompile string to bytecode */
                   const char *codestr)         /* pass result to PP_Run_Bytecode */
{
    int start;
    Py_Initialize();
    switch (mode) {
    case PP_STATEMENT:
        start = Py_file_input; break;
    case PP_EXPRESSION:
        start = Py_eval_input; break;
    default:
        start = Py_single_input;  /* prints expr results */
    }
    return Py_CompileString(codestr, "<PP_Compile_Codestr>", start);
}


int
PP_Run_Bytecode(PyObject *codeobj,           /* run compiled bytecode object */
                const char     *modname,           /* in named module's namespace */
                const char     *resfmt, void *restarget)
{
    PyObject *presult, *module, *dict;

    if (! PyCode_Check(codeobj))             /* make sure it's bytecode */
        return -1;
    module = PP_Load_Module(modname);        /* get module, init python */
    if (module == NULL)                      /* not incref'd */
        return -1;
    dict = PyModule_GetDict(module);         /* get dict namespace */
    if (dict == NULL)                        /* not incref'd */
        return -1;
    if (PP_DEBUG)
        presult = PP_Debug_Bytecode(codeobj, dict);        /* run in pdb */
    else
        presult = PyEval_EvalCode((PyCodeObject *)codeobj, dict, dict);
    return PP_Convert_Result(presult, resfmt, restarget);  /* expr val to C */
}


/**************************************************************************
 * subtle things:
 * 1) pdb.run and pdb.runeval both accept either a string or a
 * compiled code object, just because they call the built in exec and 
 * eval(), which allow either form;  further, eval() works on code
 * objects compiled as either expressions or statements, but returns
 * None as the result of statements, so we don't need to distinguish 
 * between expressions and statements here again for bytecode (we 
 * did when compiling); the equivalents in Python code:
 *     >>> a = 1
 *     >>> s = compile('x = 1', '', 'exec')
 *     >>> e = compile('a + 1', '', 'eval')
 *     >>> print eval(e)
 *     2
 *     >>> print eval(s)
 *     None
 * on the other hand, we can't blindly use pdb.runeval when dealing  
 * with uncompiled strings, because eval() fails on statement strings;
 *
 * 2) in 1.5, if you debug a string or bytecode object in a module's
 * namespace where you've already debugged once, you may see a bogus
 * return value on entry which is left over from a prior debug; this
 * is because pdb leaves a '__return__' attribute in the module's
 * dictionary, which may be a pdb bug, but we work around it here by
 * manually deleting __return__ if present before starting pdb again;
 * only happens for strings--function namespaces aren't persistent;
 **************************************************************************/


static void fixPdbRetval(PyObject *moddict) 
    { if (PyDict_DelItemString(moddict, "__return__")) PyErr_Clear(); }


PyObject *
PP_Debug_Codestr(PPStringModes mode, const char *codestring, PyObject *moddict)
{
    int res;
    PyObject *presult;
    const char *pdbname = (mode == PP_EXPRESSION ? "runeval" : "run");
    fixPdbRetval(moddict);
                                      /* pass code to a pbd.py function    */
    res = PP_Run_Function(            /* "pdb.run(stmt, gdict, ldict)"     */
             "pdb",    pdbname,       /* "pdb.runeval(expr, gdict, ldict)" */
             "O",      &presult,
             "(sOO)",  codestring, moddict, moddict); 
    return (res != 0) ? NULL : presult;     /* return null or increfd object */
}


PyObject *
PP_Debug_Bytecode(PyObject *codeobject, PyObject *moddict)
{
    int res;
    PyObject *presult;
    fixPdbRetval(moddict);
    res = PP_Run_Function(            /* "pdb.runeval(codeobj, gdict, ldict)" */
             "pdb",    "runeval",     /* accepts string|code, code=stmt|expr  */
             "O",      &presult,
             "(OOO)",  codeobject, moddict, moddict); 
    return (res != 0) ? NULL : presult;     /* null if error in run_function */
}




