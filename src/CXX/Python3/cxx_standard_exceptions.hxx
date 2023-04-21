#if !defined( PYCXX_STANDARD_EXCEPTION )
#pragma error( "define PYCXX_STANDARD_EXCEPTION before including" )
#endif

// taken for python3.5 documentation

// EnvironmentError and IOError are not used in Python3
//PYCXX_STANDARD_EXCEPTION( EnvironmentError,     QQQ )
//PYCXX_STANDARD_EXCEPTION( IOError,              QQQ )

// 5.4 Exception hierarchy
PYCXX_STANDARD_EXCEPTION( SystemExit,           BaseException )
PYCXX_STANDARD_EXCEPTION( KeyboardInterrupt,    BaseException )
PYCXX_STANDARD_EXCEPTION( GeneratorExit,        BaseException )
#if !defined( PYCXX_6_2_COMPATIBILITY )
PYCXX_STANDARD_EXCEPTION( Exception,            BaseException )
#endif
PYCXX_STANDARD_EXCEPTION(     StopIteration,        Exception )
#if !defined(MS_WINDOWS) && ((defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x03050000 && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5) || (!defined( Py_LIMITED_API )  && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5))
PYCXX_STANDARD_EXCEPTION(     StopAsyncIteration,       Exception )
#endif
// Windows builds of python 3.5 do not export the symbol PyExc_StopAsyncIteration - need atleast 3.6
#if defined(MS_WINDOWS) && ((defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x03050000 && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6) || (!defined( Py_LIMITED_API )  && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5))
PYCXX_STANDARD_EXCEPTION(     StopAsyncIteration,       Exception )
#endif
PYCXX_STANDARD_EXCEPTION(     ArithmeticError,      Exception )
PYCXX_STANDARD_EXCEPTION(         FloatingPointError,   ArithmeticError )
PYCXX_STANDARD_EXCEPTION(         OverflowError,        ArithmeticError )
PYCXX_STANDARD_EXCEPTION(         ZeroDivisionError,    ArithmeticError )
PYCXX_STANDARD_EXCEPTION(     AssertionError,       Exception )
PYCXX_STANDARD_EXCEPTION(     AttributeError,       Exception )
PYCXX_STANDARD_EXCEPTION(     BufferError,      Exception )
PYCXX_STANDARD_EXCEPTION(     EOFError,     Exception )
PYCXX_STANDARD_EXCEPTION(     ImportError,      Exception )
PYCXX_STANDARD_EXCEPTION(     LookupError,      Exception )
PYCXX_STANDARD_EXCEPTION(         IndexError,       LookupError )
PYCXX_STANDARD_EXCEPTION(         KeyError,         LookupError )
PYCXX_STANDARD_EXCEPTION(     MemoryError,      Exception )
PYCXX_STANDARD_EXCEPTION(     NameError,        Exception )
PYCXX_STANDARD_EXCEPTION(         UnboundLocalError,    NameError )
PYCXX_STANDARD_EXCEPTION(     OSError,      Exception )
#if (defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x03060000 && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6) || (!defined( Py_LIMITED_API )  && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 4)
PYCXX_STANDARD_EXCEPTION(         BlockingIOError,  OSError )
PYCXX_STANDARD_EXCEPTION(         ChildProcessError,OSError )
PYCXX_STANDARD_EXCEPTION(         ConnectionError,  OSError )
PYCXX_STANDARD_EXCEPTION(             BrokenPipeError,          ConnectionError )
PYCXX_STANDARD_EXCEPTION(             ConnectionAbortedError,   ConnectionError )
PYCXX_STANDARD_EXCEPTION(             ConnectionRefusedError,   ConnectionError )
PYCXX_STANDARD_EXCEPTION(             ConnectionResetError,     ConnectionError )
PYCXX_STANDARD_EXCEPTION(         FileExistsError,      OSError )
PYCXX_STANDARD_EXCEPTION(         FileNotFoundError,    OSError )
PYCXX_STANDARD_EXCEPTION(         InterruptedError,     OSError )
PYCXX_STANDARD_EXCEPTION(         IsADirectoryError,    OSError )
PYCXX_STANDARD_EXCEPTION(         NotADirectoryError,   OSError )
PYCXX_STANDARD_EXCEPTION(         PermissionError,      OSError )
PYCXX_STANDARD_EXCEPTION(         ProcessLookupError,   OSError )
PYCXX_STANDARD_EXCEPTION(         TimeoutError,         OSError )
#endif
PYCXX_STANDARD_EXCEPTION(     ReferenceError,   Exception )
PYCXX_STANDARD_EXCEPTION(     RuntimeError,     Exception )
PYCXX_STANDARD_EXCEPTION(         NotImplementedError,  RuntimeError )
#if !defined(MS_WINDOWS) && ((defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x03050000 && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5) || (!defined( Py_LIMITED_API )  && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5))
PYCXX_STANDARD_EXCEPTION(         RecursionError,       RuntimeError )
#endif
// Windows builds of python 3.5 do not export the symbol PyExc_RecursionError - need atleast 3.6
#if defined(MS_WINDOWS) && ((defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x03050000 && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6) || (!defined( Py_LIMITED_API )  && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 5))
PYCXX_STANDARD_EXCEPTION(         RecursionError,       RuntimeError )
#endif
PYCXX_STANDARD_EXCEPTION(     SyntaxError,      Exception )
PYCXX_STANDARD_EXCEPTION(         IndentationError,     SyntaxError )
PYCXX_STANDARD_EXCEPTION(             TabError,             IndentationError )
PYCXX_STANDARD_EXCEPTION(     SystemError,      Exception )
PYCXX_STANDARD_EXCEPTION(     TypeError,        Exception )
PYCXX_STANDARD_EXCEPTION(     ValueError,       Exception )
PYCXX_STANDARD_EXCEPTION(         UnicodeError,         ValueError )
PYCXX_STANDARD_EXCEPTION(             UnicodeDecodeError,   UnicodeError )
PYCXX_STANDARD_EXCEPTION(             UnicodeEncodeError,   UnicodeError )
PYCXX_STANDARD_EXCEPTION(             UnicodeTranslateError,UnicodeError )
