/*************************************************************************
 * PPEMBED, VERSION 2.0
 * AN ENHANCED PYTHON EMBEDDED-CALL INTERFACE 
 *
 * Wraps Python's run-time embedding API functions for easy use.
 * Most utilities assume the call is qualified by an enclosing module
 * (namespace). The module can be a file-name reference or a dummy module
 * created to provide a namespace for file-less strings. These routines
 * automate debugging, module (re)loading, input/output conversions, etc.
 *
 * Python is automatically initialized when the first API call occurs.
 * Input/output conversions use the standard Python conversion format
 * codes (described in the C API manual).  Errors are flagged as either 
 * a -1 int, or a NULL pointer result.  Exported names use a PP_ prefix
 * to minimize clashes; names in the built-in Python API use Py prefixes
 * instead (alas, there is no "import" equivalent in C, just "from*").  
 * Also note that the varargs code here may not be portable to certain
 * C compilers; to do it portably, see the text or file 'vararg.txt' 
 * here, or search for string STDARG in Python's source code files.
 *
 * New in this version/edition: names now have a PP_ prefix, files
 * renamed, compiles to a single .a file, fixed pdb retval bug for 
 * strings, and char* results returned by the "s" convert code now 
 * point to new char arrays which the caller should free() when no 
 * longer needed (this was a potential bug in prior version).  Also 
 * added new API interfaces for fetching exception info after errors, 
 * precompiling code strings to byte code, and calling simple objects.
 *
 * Also fully supports Python 1.5 module package imports: module names 
 * in this API can take the form "package.package.[...].module", where 
 * Python maps the package names to a nested directories path in your 
 * file system hierarchy;  package dirs all contain __init__.py files,
 * and the leftmost one is in a directory found on PYTHONPATH. This
 * API's dynamic reload feature also works for modules in packages;
 * Python stores the full path name in the sys.modules dictionary.
 *
 * Caveats: there is no support for advanced things like threading or 
 * restricted execution mode here, but such things may be added with 
 * extra Python API calls external to this API (see the Python/C API 
 * manual for C-level threading calls; see modules rexec and bastion 
 * in the library manual for restricted mode details).  For threading,
 * you may also be able to get by with C threads and distinct Python 
 * namespaces per Python code segments, or Python language threads 
 * started by Python code run from C (see the Python thread module).
 * 
 * Note that Python can only reload Python modules, not C extensions,
 * but it's okay to leave the dynamic reload flag on even if you might 
 * access dynamically-loaded C extension modules--in 1.5.2, Python
 * simply resets C extension modules to their initial attribute state 
 * when reloaded, but doesn't actually reload the C extension file.
 *************************************************************************/

/*
PPEMBED, VERSION 2.0
AN ENHANCED PYTHON EMBEDDED-CALL INTERFACE

Copyright 1996-2000, by Mark Lutz, and O'Reilly and Associates.
Permission to use, copy, modify, and distribute this software 
for any purpose and without fee is hereby granted.  This software
is provided on an as is basis, without warranties of any kind.
*/

#ifndef PPEMBED_H
#define PPEMBED_H

#ifdef __cplusplus
extern "C" {             /* a C library, but callable from C++ */
#endif     

#include <stdio.h>
// Python
#if defined (_POSIX_C_SOURCE)
#	undef  _POSIX_C_SOURCE
#endif // (re-)defined in pyconfig.h
#include <Python.h>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

extern int PP_RELOAD;    /* 1=reload py modules when attributes referenced */
extern int PP_DEBUG;     /* 1=start debugger when string/function/member run */

typedef enum {
     PP_EXPRESSION,      /* which kind of code-string */
     PP_STATEMENT        /* expressions and statements differ */
} PPStringModes;


/***************************************************/
/*  ppembed-modules.c: load,access module objects  */
/***************************************************/

extern const char *PP_Init(const char *modname);
extern int         PP_Make_Dummy_Module(const char *modname);
extern PyObject   *PP_Load_Module(const char *modname);
extern PyObject   *PP_Load_Attribute(const char *modname, const char *attrname);
extern int         PP_Run_Command_Line(const char *prompt);


/**********************************************************/
/*  ppembed-globals.c: read,write module-level variables  */
/**********************************************************/

extern int
    PP_Convert_Result(PyObject *presult, const char *resFormat, void *resTarget);

extern int 
    PP_Get_Global(const char *modname, const char *varname, const char *resfmt, void *cresult);

extern int
    PP_Set_Global(const char *modname, const char *varname, const char *valfmt, ... /*val*/);


/***************************************************/
/*  ppembed-strings.c: run strings of Python code  */
/***************************************************/

extern int                                         /* run C string of code */
    PP_Run_Codestr(PPStringModes mode,             /* code=expr or stmt?  */
                   const char *code,   const char *modname,    /* codestr, modnamespace */
                   const char *resfmt, void *cresult);   /* result type, target */

extern PyObject*
    PP_Debug_Codestr(PPStringModes mode,           /* run string in pdb */
                     const char *codestring, PyObject *moddict);

extern PyObject *
    PP_Compile_Codestr(PPStringModes mode, 
                       const char *codestr);             /* precompile to bytecode */

extern int
    PP_Run_Bytecode(PyObject *codeobj,             /* run a bytecode object */
                    const char     *modname, 
                    const char     *resfmt, void *restarget);

extern PyObject *                                  /* run bytecode under pdb */
    PP_Debug_Bytecode(PyObject *codeobject, PyObject *moddict); 


/*******************************************************/
/*  ppembed-callables.c: call functions, classes, etc. */
/*******************************************************/

extern BaseExport int                                       /* mod.func(args) */
    PP_Run_Function(const char *modname, const char *funcname,          /* func|classname */
                    const char *resfmt,  void *cresult,           /* result target  */
                    const char *argfmt,  ... /* arg, arg... */ ); /* input arguments*/

extern PyObject*
    PP_Debug_Function(PyObject *func, PyObject *args);   /* call func in pdb */

extern int
    PP_Run_Known_Callable(PyObject *object,              /* func|class|method */
                          const char *resfmt, void *restarget, /* skip module fetch */
                          const char *argfmt, ... /* arg,.. */ );


/**************************************************************/
/*  ppembed-attributes.c: run object methods, access members  */
/**************************************************************/

extern int 
    PP_Run_Method(PyObject *pobject, const char *method,     /* uses Debug_Function */
                      const char *resfmt,  void *cresult,              /* output */
                      const char *argfmt,  ... /* arg, arg... */ );    /* inputs */

extern int 
    PP_Get_Member(PyObject *pobject, const char *attrname,
                      const char *resfmt,  void *cresult);             /* output */

extern int 
    PP_Set_Member(PyObject *pobject, const char *attrname,
                      const char *valfmt,  ... /* val, val... */ );    /* input */


/**********************************************************/
/*  ppembed-errors.c: get exception data after api error  */ 
/**********************************************************/

extern void PP_Fetch_Error_Text();    /* fetch (and clear) exception */

extern char PP_last_error_type[];     /* exception name text */
extern char PP_last_error_info[];     /* exception data text */
extern char PP_last_error_trace[];    /* exception traceback text */

extern PyObject *PP_PyDict_Object;    /* saved PyDict object */
extern PyObject *PP_last_traceback;   /* saved exception traceback object */
extern PyObject *PP_last_exception_type;   /* saved exception type */


#ifdef __cplusplus
}
#endif

#endif /*PREEMBED_H*/
