#include "misc/SoDBP.h"

#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/sensors/SoTimerSensor.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_3DS_IMPORT_CAPABILITIES
#include "3ds/3dsLoader.h"
#endif // HAVE_3DS_IMPORT_CAPABILITIES

#include "fields/SoGlobalField.h"
#include "coindefs.h"

#ifdef COIN_THREADSAFE
// need to include SbRWMutex.h to make C++ call the actual destructor,
// and not just default destructor
#include <Inventor/threads/SbRWMutex.h>
SbRWMutex * SoDBP::globalmutex = NULL;
#endif // COIN_THREADSAFE
SbList<SoDB_HeaderInfo *> * SoDBP::headerlist = NULL;
SoSensorManager * SoDBP::sensormanager = NULL;
SoTimerSensor * SoDBP::globaltimersensor = NULL;
UInt32ToInt16Map * SoDBP::converters = NULL;
SbBool SoDBP::isinitialized = FALSE;
int SoDBP::notificationcounter = 0;
SbList<SoDBP::ProgressCallbackInfo> * SoDBP::progresscblist = NULL;

// *************************************************************************
// FIXME: this should be moved into a function in tidsbits.c. 20050509 mortene.

// A sanity check. Invoked from SoDB::init() below.
//
// This checks that the undefined behaviour of the system's
// vsnprintf() works the way we need it to when the (C99) va_copy()
// macro is not available on the system. See coin_vsnprintf() in
// tidbits.c for more information.
//
// The relevant part of ISO/IEC 9899:1999 ("C99") says (in section
// 7.19.6.12):
//
//  2] The vsnprintf function is equivalent to snprintf, with the
//  variable argument list replaced by arg, which shall have been
//  initialized by the va_start macro (and possibly subsequent va_arg
//  calls). The vsnprintf function does not invoke the va_end
//  macro. If copying takes place between objects that overlap, the
//  behavior is undefined.

static void
forward_sprintf(char * dst, unsigned int realdstlen, const char * fmtstr, ...)
{
  va_list args;
  va_start(args, fmtstr);

  // This first call is just to get one invocation of va_arg() done
  // from the system's vsnprintf().
  int len = coin_vsnprintf(dst, (unsigned int)(strlen(fmtstr) - 1), fmtstr, args);
  assert(len == -1);

  // The next call is made to see whether or not additional
  // invocations of vsnprintf() on the system without an intervening
  // va_start()/va_end() pair will cause va_arg() to start picking up
  // arguments after the end of the actual argument list.
  len = coin_vsnprintf(dst, realdstlen, fmtstr, args);
  assert(len != -1);

  va_end(args);
}

void
SoDBP::variableArgsSanityCheck(void)
{
  const char * fmtstr = "abc%s";
  char block[128];

  // Notice that there's only a single "%s" in the format string, but
  // we pass in *two* string arguments to test whether or not the last
  // will be erroneously used.
  forward_sprintf(block, sizeof(block) - 1, fmtstr, "-XXX", "-YYY");

  if (strcmp(block, "abc-XXX") == 0) { return; } // all ok

  // Make sure the test itself is correct.
  assert((strcmp(block, "abc-YYY") == 0) && "variable args sanity check bogus");

  // SoDebugError not initialized yet, so use printf().
  (void)printf("Coin " COIN_VERSION ": Sanity Check Report\n\n");
  (void)printf("This system's vsnprintf() has a variable argument\n"
               "invocation which is not working properly with Coin.\n\n"
               "Application will continue to run, but be aware that this\n"
               "problem could cause obscure bugs.\n\n"
               "Please report this problem to <coin-support@coin3d.org>\n"
               "for further assistance.\n\n");
}

// This will free all resources which have been allocated by the SoDB
// class. This isn't really necessary in user applications (the
// underlying operating system will take care of making the resources
// available again), but it is useful for debugging purposes -- it
// makes it easier to find memory leaks.
void
SoDBP::clean(void)
{
  delete SoDBP::progresscblist;
  SoDBP::progresscblist = NULL;

  // Avoid having the SoSensorManager instance trigging the callback
  // into the So@Gui@ class -- not only have it possible "died", but
  // the whole GUI toolkit could have died until we come here.
  //
  // (This has already proven itself to be a source of problems with
  // the SoQt library, which wets its pants on the
  // SoDBP::globaltimersensor destruction under Microsoft Windows if we don't
  // first nullify the callback function pointer.)
  SoDBP::sensormanager->setChangedCallback(NULL, NULL);

  delete SoDBP::globaltimersensor;
  SoDBP::globaltimersensor = NULL;
  delete SoDBP::converters;
  SoDBP::converters = NULL;

  delete SoDBP::sensormanager;
  SoDBP::sensormanager = NULL;

  for (int i = 0; i < SoDBP::headerlist->getLength(); i++)
    delete (*SoDBP::headerlist)[i];
  delete SoDBP::headerlist;
  SoDBP::headerlist = NULL;

#ifdef COIN_THREADSAFE
  delete SoDBP::globalmutex;
  SoDBP::globalmutex = NULL;
#endif // COIN_THREADSAFE
}

void
SoDBP::removeRealTimeFieldCB(void)
{
  SoGlobalField * field = SoGlobalField::getGlobalFieldContainer("realTime");

//  SoGlobalField::removeGlobalFieldContainer(field);
  /* 
    The above function never unref's the field because in the SoGlobalField class
	the allcontainers list is specifically set to disable referencing. Therefore to
	delete that actual field we have to unreference it. The unreferencing then
	automatically removes it from the containers list, so the above call has to be
	removed. RHW 20141006
  */
  field->unref ();
}

// This is the timer sensor callback which updates the realTime global
// field.
void
SoDBP::updateRealTimeFieldCB(void * COIN_UNUSED_ARG(data), SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoField * f = SoDB::getGlobalField("realTime");
  if (f && (f->getTypeId() == SoSFTime::getClassTypeId())) {
    ((SoSFTime *)f)->setValue(SbTime::getTimeOfDay());
  }
}

SbBool
SoDBP::is3dsFile(SoInput * in)
{
  if (in->getNumBytesRead() > 0) { return FALSE; }
  if (in->getHeader().getLength() > 0) { return FALSE; }

  char c1, c2;
  if (!in->get(c1)) { return FALSE; }
  if (!in->get(c2)) { in->putBack(c1); return FALSE; }
  in->putBack(c2);
  in->putBack(c1);

  // FIXME: this seems like a rather weak test for 3D Studio format
  // files (1 out of every 65536 files with random data would return
  // TRUE). Should check up on the format spec, and improve
  // it. 20031117 mortene.
  if (c1 != 0x4d) { return FALSE; }
  if (c2 != 0x4d) { return FALSE; }

  return TRUE;
}

#if defined(HAVE_WINDLL_RUNTIME_BINDING) && defined(HAVE_TLHELP32_H)

#ifdef HAVE_WINDOWS_H
#include <windows.h> // WINAPI
#endif // HAVE_WINDOWS_H

#include <tlhelp32.h>

typedef HANDLE (WINAPI * CreateToolhelp32Snapshot_t)(DWORD, DWORD);
typedef BOOL (WINAPI * Module32First_t)(HANDLE, LPMODULEENTRY32);
typedef BOOL (WINAPI * Module32Next_t)(HANDLE, LPMODULEENTRY32);

static CreateToolhelp32Snapshot_t funCreateToolhelp32Snapshot;
static Module32First_t funModule32First;
static Module32Next_t funModule32Next;

void
SoDBP::listWin32ProcessModules(void)
{
  BOOL ok;

  HINSTANCE kernel32dll = LoadLibrary("kernel32.dll");
  assert(kernel32dll && "LoadLibrary(''kernel32.dll'') failed");

  funCreateToolhelp32Snapshot = (CreateToolhelp32Snapshot_t)
    GetProcAddress(kernel32dll, "CreateToolhelp32Snapshot");
  funModule32First = (Module32First_t)
    GetProcAddress(kernel32dll, "Module32First");
  funModule32Next = (Module32Next_t)
    GetProcAddress(kernel32dll, "Module32Next");

  if (!funCreateToolhelp32Snapshot || !funModule32First || !funModule32Next) {
    SoDebugError::postWarning("SoDBP::listWin32ProcessModules",
                              "Tool Help Library not available (NT4?)");
  }
  else {
    HANDLE tool32snap = funCreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    assert((tool32snap != (void *)-1) && "CreateToolhelp32Snapshot() failed");

    MODULEENTRY32 moduleentry;
    moduleentry.dwSize = sizeof(MODULEENTRY32);
    ok = funModule32First(tool32snap, &moduleentry);
    assert(ok && "Module32First() failed");

    SoDebugError::postInfo("SoDBP::listWin32ProcessModules",
                           "MODULEENTRY32.szModule=='%s', .szExePath=='%s'",
                           moduleentry.szModule, moduleentry.szExePath);

    while (funModule32Next(tool32snap, &moduleentry)) {
      SoDebugError::postInfo("SoDBP::listWin32ProcessModules",
                             "MODULEENTRY32.szModule=='%s', .szExePath=='%s'",
                             moduleentry.szModule, moduleentry.szExePath);
    }

    assert(GetLastError()==ERROR_NO_MORE_FILES && "Module32Next() failed");

    ok = CloseHandle(tool32snap);
    assert(ok && "CloseHandle() failed");
  }

  ok = FreeLibrary(kernel32dll);
  assert(ok && "FreeLibrary() failed");
}


#else // !HAVE_WINDLL_RUNTIME_BINDING || !HAVE_TLHELP32_H

void
SoDBP::listWin32ProcessModules(void)
{
  SoDebugError::postWarning("SoDBP::listWin32ProcessModules",
                            "Tool Help Library not available "
                            "(non-win32 platform?)");
}

#endif // !HAVE_WINDLL_RUNTIME_BINDING || !HAVE_TLHELP32_H

SoSeparator *
SoDBP::read3DSFile(SoInput * in)
{
  assert(SoDBP::is3dsFile(in));

#ifdef HAVE_3DS_IMPORT_CAPABILITIES

  SoSeparator * b;
  const SbBool ok = coin_3ds_read_file(in, b);
  if (ok) { return b; }

#else // !HAVE_3DS_IMPORT_CAPABILITIES

  SoDebugError::postWarning("SoDB::read",
                            "It seems like the input file is in 3D Studio "
                            "format, but this configuration of Coin was "
                            "built without support for that file format.");

#endif // !HAVE_3DS_IMPORT_CAPABILITIES

  return NULL;
}


void
SoDBP::progress(const SbName & itemid,
                float fraction,
                SbBool interruptible)
{
  if (SoDBP::progresscblist != NULL) {
    for (int i = 0; i < SoDBP::progresscblist->getLength(); i++) {
      SoDBP::ProgressCallbackInfo info = (*SoDBP::progresscblist)[i];
      info.func(itemid, fraction, interruptible, info.userdata);
    }
  }
}
