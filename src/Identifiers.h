#ifndef DEF_IDENTIFIERS
#define DEF_IDENTIFIERS

#include <string>

using namespace std;

enum Rat {E_UTRA, UTRA, GERAN_CS, GERAN_PS, CDMA2000};

struct Imsi{
  string mcc;
  string mnc;
  string msin;
};

#endif
