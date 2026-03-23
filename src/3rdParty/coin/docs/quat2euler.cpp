/*
  Merge this code into SbRotation.cpp as SbRotation::getAsEuler(). Test!
 */

#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbRotation.h>

void
matrix2euler(float mat[16], float euler[3])
{
  // adapted from flipcode FAQ
  // http://www.flipcode.com/documents/matrfaq.html#Q37
  euler[1] = asin(mat[2]);
  float C = cos( euler[1] );

  float tx, ty;

  if (fabs(C) > 0.005f) { // Gimball lock?
    tx = mat[10] / C;
    ty = -mat[6] / C;

    euler[0] = atan2(ty, tx);

    tx = mat[0] / C;
    ty = -mat[1] / C;

    euler[2] = atan2(ty, tx);
  }
  else {
    euler[0] = 0.0f;
    tx = mat[5];
    ty = mat[4];
    euler[2] = atan2(ty, tx);
  }
}

void
quat2matrix(float quat[4], float matrix[16])
{
  // adapted from Coin
  const float x = quat[0];
  const float y = quat[1];
  const float z = quat[2];
  const float w = quat[3];

  matrix[0] = w*w + x*x - y*y - z*z;
  matrix[4] = 2.0f*x*y + 2.0f*w*z;
  matrix[8] = 2.0f*x*z - 2.0f*w*y;
  matrix[12] = 0.0f;

  matrix[1] = 2.0f*x*y-2.0f*w*z;
  matrix[5] = w*w - x*x + y*y - z*z;
  matrix[9] = 2.0f*y*z + 2.0f*w*x;
  matrix[13] = 0.0f;

  matrix[2] = 2.0f*x*z + 2.0f*w*y;
  matrix[6] = 2.0f*y*z - 2.0f*w*x;
  matrix[10] = w*w - x*x - y*y + z*z;
  matrix[14] = 0.0f;

  matrix[3] = 0.0f;
  matrix[7] = 0.0f;
  matrix[11] = 0.0f;
  matrix[15] = w*w + x*x + y*y + z*z;
}

void
quat2euler(float quat[4], float euler[3])
{
  float m[16];
  quat2matrix(quat, m);
  matrix2euler(m, euler);
}

float to_angle(float rad)
{
  float angle = rad * 180.0f / float(M_PI);
  return angle;
}

void print_euler(const SbRotation & rot)
{
  float euler[3];
  float quat[4];
  const float * val = rot.getValue();
  quat[0] = val[0];
  quat[1] = val[1];
  quat[2] = val[2];
  quat[3] = val[3];

  quat2euler(quat, euler);
  fprintf(stderr,"rot: %g %g %g\n",
          to_angle(euler[0]),
          to_angle(euler[1]),
          to_angle(euler[2]));
}

int main(int argc, char ** argv)
{
  SbRotation rot = SbRotation::identity();
  print_euler(rot);

  rot.setValue(SbVec3f(0,1,0), M_PI/2);
  print_euler(rot);

  rot.setValue(SbVec3f(1,0,0), M_PI/2);
  print_euler(rot);

  rot.setValue(SbVec3f(0,0,1), M_PI/2);
  print_euler(rot);
}
