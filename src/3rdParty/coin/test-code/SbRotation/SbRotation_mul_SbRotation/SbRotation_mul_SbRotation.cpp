/************************************************************************
 *
 * SbRotation & SbRotation::operator*=(SbRotation &)
 *
 ************************************************************************/

#include <stdio.h>
#include <Inventor/SoDB.h>
#include <Inventor/SbLinear.h>


int
main(int argc, char ** argv)
{
  SoDB::init();

  // FIXME: istty(stdin). 20010114 mortene.

  int valsread;
  SbRotation r = SbRotation::identity();
  do {
    float v[4];
    valsread = fscanf(stdin, "%f %f %f %f\n", &v[0], &v[1], &v[2], &v[3]);

    if (valsread == 4) {
      SbRotation n(v);
      r *= n;
      r = r * n; // test this variant aswell
      const float * a = r.getValue();
      (void)fprintf(stdout, "%.3f %.3f %.3f %3f\n", a[0], a[1], a[2], a[3]);
    }
  } while (valsread == 4);

  if (valsread != EOF) {
    (void)fflush(stdout);
    (void)fprintf(stderr, "Input data format error!\n");
  }

  return 0;
}
