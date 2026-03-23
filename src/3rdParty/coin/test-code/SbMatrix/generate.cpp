/************************************************************************
 *
 * Generate a set of "valid" matrices (i.e. matrices put together from
 * a set of valid translations, scalevectors and rotations) on stdout.
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <Inventor/SoDB.h>
#include <Inventor/SbLinear.h>

float
rndf(void)
{
  return (float(rand()) / float(RAND_MAX) - 0.5f) * 4.0f;
}

int
main(int argc, char ** argv)
{
  if (argc != 2) {
    (void)fprintf(stderr,
                  "\n\n\tUsage: %s NUM\n\n"
                  "\tNUM = number of matrices to output.\n\n",
                  argv[0]);
    exit(1);
  }

  SoDB::init();

  srand(19720408);
  int num = atoi(argv[1]);
  for (int i=0; i < num; i++) {
    SbMatrix m;
    m.setTransform(// translation
                   SbVec3f(rndf(), rndf(), rndf()),
                   // rotation
                   SbRotation(SbVec3f(rndf(), rndf(), rndf()), rndf()),
                   // scalevec
                   SbVec3f(rndf(), rndf(), rndf()),
                   // scaleorientation
                   SbRotation(SbVec3f(rndf(), rndf(), rndf()), rndf()),
                   // center
                   SbVec3f(rndf(), rndf(), rndf()));

    (void)fprintf(stdout,
                  "%.3f %.3f %.3f %.3f "
                  "%.3f %.3f %.3f %.3f "
                  "%.3f %.3f %.3f %.3f "
                  "%.3f %.3f %.3f %.3f\n",
                  m[0][0], m[0][1], m[0][2], m[0][3],
                  m[1][0], m[1][1], m[1][2], m[1][3],
                  m[2][0], m[2][1], m[2][2], m[2][3],
                  m[3][0], m[3][1], m[3][2], m[3][3]);
  }

  return 0;
}
