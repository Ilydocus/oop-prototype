#include "Identifiers.hh"

void genRandId(string * id,const int len){
  char temp[len];
  static const char num[11] = "0123456789";
  for (int i = 0; i<len; ++i){
    temp[i] = num[(int)((double)rand() / ((double)RAND_MAX + 1)*(sizeof(num)-1))];
  }
  temp[len]=0;
  string s(temp);
  *id = s;
}
