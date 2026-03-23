/************************************************************************
 *
 * Generate a set of quaternions on stdout.
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
                  "\tNUM = number of quaternions to output.\n\n",
                  argv[0]);
    exit(1);
  }

  SoDB::init();

  srand(19720408);
  int num = atoi(argv[1]);
  for (int i=0; i < num; i++) {
    SbRotation q(rndf(), rndf(), rndf(), rndf());
    float x, y, z, w;
    q.getValue(x, y, z, w);
    (void)fprintf(stdout, "%.3f %.3f %.3f %.3f\n", x, y, z, w);
  }

  return 0;
}
