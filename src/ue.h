#ifndef DEF_UE
#define DEF_UE

#include <string>
#include "RrcMessages.pb.h"

using namespace std;

struct UeContext_ue{
  Imsi_message imsi;
  string srbId;
  int securityKey;
};

void genImsi (Imsi_message *imsi);
void genRandId(string * id,const int len);


#endif
