
#include "amici/symbolic_functions.h"
#include "amici/defines.h" //realtype definition
typedef amici::realtype realtype;
#include <cmath> 

using namespace amici;

void sigma_y_model_robertson(double *sigmay, const realtype t, const realtype *p, const realtype *k) {
  sigmay[0] = 1.0;
  sigmay[1] = 1.0;
  sigmay[2] = 1.0;
}

