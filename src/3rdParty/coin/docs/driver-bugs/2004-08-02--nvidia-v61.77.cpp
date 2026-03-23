/*
  The simple example below causes a WinXP machine with NVIDIA drivers
  to crash within the driver. The example code just compiles a GL
  display list with a few triangles, then invokes this list. This
  causes a crash.

  System information:

  * Driver versions found to crash: 52.16, 56.64, 56.72, 61.76, 61.77
    (no others were tested).

  * GeForce4 Ti 4600, GeForce2 Ultra, GeForce FX 5900, GeForce FX
    5900XT (all that were tested).

  * Win XP Service Pack 1, Win 2000 Service Pack 4 (all tested
    platforms).

  * Display settings: any resolution, any color depth.

  <mortene@sim.no>.
*/
/*
  UPDATE 2004-08-31 mortene:

  This stand-alone example program has also been found to crash on
  Linux, with Linux driver versions 53.36 and 61.11 (the latter one is
  the most recent, as of 2004-08-31).

  The Linux crash happens with a backtrace ending in memcpy(), and the
  next level up is inside libGL.so.
*/

// *************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#endif // _WIN32

#include <GL/glut.h>

static void
send_triangle(void)
{
  const float v[3][3] = { { 0, -1, 0 }, { 1, -1, 0 }, { 1, 0, 0 } };

  for (int j=0; j < 3; j++) {
    glNormal3f(0, 0, 1);
    glVertex3f(v[j][0], v[j][1], v[j][2]);
  }
}

static void
expose_cb(void)
{
  for (int loop = 0; loop < 2048; loop++) {
    printf("loop==%d\n", loop);

    int i;

    glEnable(GL_DEPTH_TEST);
    glDrawBuffer(GL_BACK);
    glClearColor(0.000000,0.000000,0.000000,0.000000);

    glColorMaterial(GL_FRONT_AND_BACK,GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glViewport(0,0,892,658);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    // *********************************************************************

    GLuint gllist = glGenLists(1);
    // Change to GL_COMPILE, and the crashes go away:
    glNewList(gllist,GL_COMPILE_AND_EXECUTE);

    glColor4f(0, 1, 0, 1);

    glBegin(GL_TRIANGLES);
    const int nr = ((float)rand() / RAND_MAX) * 32;
    printf("nr==%d\n", nr);
    for (i=0; i < nr; i++) { send_triangle(); }
    glColor4f(1, 0, 0, 1);
    send_triangle();
    glEnd();

    glEndList();

    // *********************************************************************

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glCallList(gllist);

    glutSwapBuffers();

#if 0 // enable this, and the crashes go away
    glDeleteLists(gllist, 1);
#endif
    printf("gllist==%d\n", gllist);
  }
}

// *************************************************************************

int
main(int argc, char ** argv)
{
  srand(argc > 1 ? atoi(argv[1]) : 0);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  glutInitWindowSize(892, 658);
  (void)glutCreateWindow("hepp");
  glutDisplayFunc(expose_cb);

  glutMainLoop();

  return 0;
}

// *************************************************************************
