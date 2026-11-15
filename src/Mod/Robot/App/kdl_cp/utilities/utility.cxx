/** @file   utility.cpp
 *  @author Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *  @version 
 *      ORO_Geometry V0.2
 *   
 *  @par history
 *   - changed layout of the comments to accommodate doxygen
 */
 
#include "utility.h"

namespace KDL {

int STREAMBUFFERSIZE = 10000;
int MAXLENFILENAME = 255;
const double PI      = 3.141592653589793238462643383279502884; // PI
const double PI_2    = 1.570796326794896619231321691639751442; // PI/2
const double PI_4    = 0.785398163397448309615660845819875721; // PI/4
const double deg2rad = 0.017453292519943295769236907684886127; // PI/180
const double rad2deg = 57.29577951308232087679815481410517033; // 180/PI
double epsilon = 1e-6;
}
