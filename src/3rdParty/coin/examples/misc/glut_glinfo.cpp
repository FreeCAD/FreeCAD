/*
  Tool that reads out GL_VENDOR, GL_VERSION and GL_EXTENSIONS and prints them to stdout.
  We use glut to open a GL context.
*/

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#endif

#include <cstdio>
#include <cstdlib>

int glutwin;

// Redraw on expose events.
void
expose_cb(void)
{
  const char * version = (const char *)glGetString(GL_VERSION);
  const char * vendor = (const char *)glGetString(GL_VENDOR);
  const char * ext = (const char *)glGetString(GL_EXTENSIONS);

  fprintf(stdout,"Vendor: %s\n", vendor);
  fprintf(stdout,"Version: %s\n", version);
  fprintf(stdout,"Extensions: %s\n", ext);

  // just exit the application immediately
  glutDestroyWindow(glutwin);
}


// ----------------------------------------------------------------------

#ifdef _WIN32

#include <windows.h>
#include <winbase.h>

int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
        int nCmdShow)
{
#else // UNIX

int
main(int argc, char ** argv)
{

#endif

#ifdef _WIN32
  int argc = 1;
  char * argv[] = { "glut_glinfo.exe", (char *) NULL };
  glutInit(&argc, argv);
#else
  glutInit(&argc, argv);
#endif

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  glutInitWindowSize(512, 400);
  glutwin = glutCreateWindow("GLInfo");
  glutDisplayFunc(expose_cb);

  glutMainLoop();

  return -1;
}
