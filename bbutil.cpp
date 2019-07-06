/*
  Copyright (C) 2012-2013 - Voidious

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <math.h>
#include <string.h>
#include <algorithm>
#include <ctime>
#include <complex>
#include <limits>
#include "bbutil.h"

double limit(double p, double q, double r) {
  return std::min(r, std::max(p, q));
}

int signum(double x) {
  if (x < 0) {
    return -1;
  } else if (x > 0) {
    return 1;
  } else {
    return 0;
  }
}

double square(double x) {
  return x * x;
}

double abs(double x) {
  if (x < 0) {
    return -x;
  }
  return x;
}

double round(double d, int x) {
  int powerTen = 1;
  for (int i = 0; i < x; i++) {
    powerTen = powerTen * 10;
  }
  return floor((d * powerTen) + .5) / powerTen;
}

double normalRelativeAngle(double x) {
  while (x > M_PI) {
    x -= (2 * M_PI);
  }
  while (x < -M_PI) {
    x += (2 * M_PI);
  }
  return x;
}

double normalAbsoluteAngle(double x) {
  while (x > 2 * M_PI) {
    x -= (2 * M_PI);
  }
  while (x < 0) {
    x += (2 * M_PI);
  }
  return x;
}

double toDegrees(double x) {
  return x * 180 / M_PI;
}

char** parseFlag(int argc, char *argv[], const char *flag, int numValues) {
  for (int x = 0; x < argc - numValues; x++) {
    char *arg = argv[x];
    if (strlen(arg) > 1 && arg[0] == '-' && strcmp(&(arg[1]), flag) == 0) {
      char **values = new char*[numValues];
      for (int y = 0; y < numValues; y++) {
        values[y] = argv[x + y + 1];
      }
      return values;
    }
  }
  return 0;
}

bool flagExists(int argc, char *argv[], const char *flag) {
  for (int x = 0; x < argc; x++) {
    char *arg = argv[x];
    if (strlen(arg) > 1 && arg[0] == '-' && strcmp(&(arg[1]), flag) == 0) {
      return true;
    }
  }
  return false;
}

bool isWhitespace(const char *s) {
  size_t len = strlen(s);
  for (int x = 0; x < len; x++) {
    if (s[x] != ' ' && s[x] != '\t' && s[x] != '\n' && s[x] != '\r') {
      return false;
    }
  }
  return true;
}

char* getTimestamp() {
  time_t t;
  struct tm *timeinfo;
  char timestamp[80];
  time(&t);
  timeinfo = localtime(&t);
  strftime(timestamp, 80, "%Y.%m.%d-%H.%M.%S", timeinfo);
  char *newTimestamp = new char[strlen(timestamp) + 1];
  strcpy(newTimestamp, timestamp);
  return newTimestamp;
}

// Tries to approximate complex number as real number 
// if possible to given eps or returns NaN
double approxReal(
    std::complex<double> zz, double epsRel, double epsAbs) {
  if (fabs(std::imag(zz)) < epsRel*abs(zz) + epsAbs) {
    return std::real(zz);
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

// Returns positive minimal value if possible or NaN 
double getPosMin(
    double aa, double bb) {
  if (aa > 0. and bb > 0.) {
    return fmin(aa,bb);
  } else if(aa > 0.) {
    return aa;
  } else if(bb > 0.){
    return bb;
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

double getPosMin(
    double aa, double bb, double cc, double dd) {
  return getPosMin(getPosMin(aa,bb),getPosMin(cc,dd));
}

// Solve quadratic equation, only return real results or NaN
void solveQuadratic(
    double aa, double bb, double cc, double *t1, double *t2) {
  double qq = bb*bb - 4.*aa*cc;
  if(qq < 0.) {
    *t1 = std::numeric_limits<double>::quiet_NaN();
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  } else if(qq > 0. && abs(aa) > 0.) {
    qq = -0.5*(bb + signum(bb)*sqrt(qq));
    *t1 = qq/aa;
    *t2 = cc/qq;
    return;
  } else if(qq == 0. && abs(aa) > 0.) {
    *t1 = -0.5*bb/aa;
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  } else if(aa == 0. && abs(bb) > 0.) {
    *t1 = -cc/bb;
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  } else {
    *t1 = std::numeric_limits<double>::quiet_NaN();
    *t2 = std::numeric_limits<double>::quiet_NaN();
    return;
  }
}

// Solve cubic equation, only return real results or NaN
std::complex<double> solveCubicHelper(
    double bb, double cc, double dd,
    std::complex<double> pp, std::complex<double> qq, std::complex<double> rr) {
  return pp - (dd+pp*(cc+pp*(bb+pp)))/((pp-qq)*(pp-rr));
}

void solveCubic(
    double aa, double bb, double cc, double dd,
    double *t1, double *t2, double *t3) {
  std::complex<double> ppold, qqold, rrold;
  std::complex<double> pp, qq, rr;
  std::complex<double> i(0.,1.);
  double epsRel = 1.e-12; // TODO Not sure if it would make more sense
  double epsAbs = 1.e-24; // to do this in a global way
  if (aa == 0.) {
    *t3 = std::numeric_limits<double>::quiet_NaN();
    solveQuadratic(bb, cc, dd, t1, t2);
  } else {
    bb = bb/aa;
    cc = cc/aa;
    dd = dd/aa;
    pp = 1.;
    qq = 0.5 + 0.1*i;
    rr = qq*qq;
    for (int ii = 0; ii <= 1000; ii++) {
      ppold = pp;
      qqold = qq;
      rrold = rr;
      pp = solveCubicHelper(bb, cc, dd, pp, qq, rr);
      qq = solveCubicHelper(bb, cc, dd, qq, rr, pp);
      rr = solveCubicHelper(bb, cc, dd, rr, pp, qq);
      if ((abs(pp-ppold) < epsRel*abs(pp) + epsAbs and 
           abs(qq-qqold) < epsRel*abs(qq) + epsAbs and
           abs(rr-rrold) < epsRel*abs(rr) + epsAbs)) {
        break;
      } else if (ii == 1000) {
        throw std::runtime_error( "not converged" );
      }
    }
    *t1 = approxReal(pp, epsRel, epsAbs);
    *t2 = approxReal(qq, epsRel, epsAbs);
    *t3 = approxReal(rr, epsRel, epsAbs);
  }
  return;
}

// Solve quartic equation, only return real results or NaN
std::complex<double> solveQuarticHelper(
    double bb, double cc, double dd, double ee,
    std::complex<double> pp, std::complex<double> qq, std::complex<double> rr, std::complex<double> ss) {
  return pp - (ee+pp*(dd+pp*(cc+pp*(bb+pp))))/((pp-qq)*(pp-rr)*(pp-ss));
}

void solveQuartic(
    double aa, double bb, double cc, double dd, double ee, 
    double *t1, double *t2, double *t3, double *t4) {
  std::complex<double> ppold, qqold, rrold, ssold;
  std::complex<double> pp, qq, rr, ss;
  std::complex<double> i(0.,1.);
  double epsRel = 1.e-12; // TODO Not sure if it would make more sense
  double epsAbs = 1.e-24; // to do this in a global way
  if(aa == 0.) {
    *t4 = std::numeric_limits<double>::quiet_NaN();
    solveCubic(bb, cc, dd, ee, t1, t2, t3);
  } else {
    bb = bb/aa;
    cc = cc/aa;
    dd = dd/aa;
    ee = ee/aa;
    pp = 1.;
    qq = 0.5 + 0.1*i;
    rr = qq*qq;
    ss = qq*rr;
    for (int ii = 0; ii <= 1000; ii++) {
      ppold = pp;
      qqold = qq;
      rrold = rr;
      ssold = ss;
      pp = solveQuarticHelper(bb, cc, dd, ee, pp, qq, rr, ss);
      qq = solveQuarticHelper(bb, cc, dd, ee, qq, rr, ss, pp);
      rr = solveQuarticHelper(bb, cc, dd, ee, rr, ss, pp, qq);
      ss = solveQuarticHelper(bb, cc, dd, ee, ss, pp, qq, rr);
      if (abs(pp-ppold) < epsRel*abs(pp) + epsAbs and 
          abs(qq-qqold) < epsRel*abs(qq) + epsAbs and
          abs(rr-rrold) < epsRel*abs(rr) + epsAbs and 
          abs(ss-ssold) < epsRel*abs(ss) + epsAbs) {
        break;
      } else if (ii == 1000) {
        throw std::runtime_error( "not converged" );
      }
    }
    *t1 = approxReal(pp, epsRel, epsAbs);
    *t2 = approxReal(qq, epsRel, epsAbs);
    *t3 = approxReal(rr, epsRel, epsAbs);
    *t4 = approxReal(ss, epsRel, epsAbs);
  }
  return;
}




// See Numerical Recipes "safe" Newton's method
int newtonBisect(rootFun fun, double* xStart, void* params, double* out, double epsRel, double epsAbs) {

  double dx, dxold;
  double ff[2];
  double fh[2];
  double fl[2];
  double temp, xh, xl, rts;
  int maxIter = 1000;

  fun(&xStart[0], params, fl);
  fun(&xStart[1], params, fh);
  if (fl[0]*fh[0] > 0.) {
    return -1;
  } else if (fl[0] == 0.) {
    out[0] = xStart[0];
    return 1;
  } else if (fh[0] == 0.) {
    out[0] = xStart[1];
    return 1;
  }
  
  if (fl[0] < 0.) {
    xl = xStart[0];
    xh = xStart[1];
  } else {
    xh = xStart[0];
    xl = xStart[1];
  }

  rts = 0.5*(xl+xh);
  dxold = abs(xh-xl);
  dx = dxold;
  fun(&rts, params, ff);
  
  for (int ii = 0; ii <= maxIter; ii++) {
    if ( (((rts-xh)*ff[1]-ff[0])*((rts-xl)*ff[1]-ff[0]) > 0.0) or 
         (abs(2.0*ff[0]) > fabs(dxold*ff[1])) ) {
      dxold = dx;
      dx = 0.5*(xh-xl);
      rts = xl+dx;
      if (xl == rts) {
        out[0] = rts;
        return 1;
      }
    } else {
      dxold = dx;
      dx = ff[0]/ff[1];
      temp = rts;
      rts -= dx;
      if (temp == rts) {
        out[0] = rts;
        return 1;
      }
    }
    if (abs(dx) < epsAbs + epsRel*abs(rts)) {
      out[0] = rts;
      return 1;
    }
        
    fun(&rts, params, ff);
    if (ff[0] < 0.0) {
      xl = rts;
    } else {
      xh = rts;
    }
  }

//    # Only reachable if ii == maxIter. Convergence failed.
    return -2;
}
