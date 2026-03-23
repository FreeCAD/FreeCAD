/************************************************************************
 *
 * SbMatrix::multLeft(SbMatrix)
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
  SbMatrix rightmatrix = SbMatrix::identity();
  do {
    SbMat m;
    valsread = fscanf(stdin,
                      "%f %f %f %f "
                      "%f %f %f %f "
                      "%f %f %f %f "
                      "%f %f %f %f\n",
                      &m[0][0], &m[0][1], &m[0][2], &m[0][3],
                      &m[1][0], &m[1][1], &m[1][2], &m[1][3],
                      &m[2][0], &m[2][1], &m[2][2], &m[2][3],
                      &m[3][0], &m[3][1], &m[3][2], &m[3][3]);

    if (valsread == 16) {
      SbMatrix leftmatrix(m);
      rightmatrix.multLeft(leftmatrix);
      for (int i=0; i < 4; i++) {
        for (int j=0; j < 4; j++) {
          (void)fprintf(stdout, "%10.3f ", rightmatrix[i][j]);
        }
        (void)fprintf(stdout, "\n");
      }
      (void)fprintf(stdout, "\n");
    }
  } while (valsread == 16);

  if (valsread != EOF) {
    (void)fflush(stdout);
    (void)fprintf(stderr, "Input data format error!\n");
  }

  return 0;
}
