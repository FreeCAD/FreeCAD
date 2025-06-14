#pragma once

#if defined(_MSC_VER)

#	if defined(__clang__)
#		if defined(_DEBUG) && defined(_WIN64)
#			pragma comment(lib, "libfastsignalsd-llvm-x64.lib")
#		elif defined(_DEBUG)
#			pragma comment(lib, "libfastsignalsd-llvm-x32.lib")
#		elif defined(_WIN64)
#			pragma comment(lib, "libfastsignals-llvm-x64.lib")
#		else
#			pragma comment(lib, "libfastsignals-llvm-x32.lib")
#		endif
#	elif _MSC_VER <= 1900
#		error this library needs Visual Studio 2017 and higher
#	elif _MSC_VER < 2000
#		if defined(_DEBUG) && defined(_WIN64)
#			pragma comment(lib, "libfastsignalsd-v141-x64.lib")
#		elif defined(_DEBUG)
#			pragma comment(lib, "libfastsignalsd-v141-x32.lib")
#		elif defined(_WIN64)
#			pragma comment(lib, "libfastsignals-v141-x64.lib")
#		else
#			pragma comment(lib, "libfastsignals-v141-x32.lib")
#		endif
#	else
#		error unknown Visual Studio version, auto-linking setup failed
#	endif

#endif
