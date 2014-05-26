#ifndef DEF_UECONTEXTMME
#define DEF_UECONTEXTMME

#include "S1Messages.pb.h"
#include "UeContext.hh"

struct UeStateMme{
  int mmeUeS1ApId;
  int securityKey_mme;
};

class UeContextMme : public UeContext
{
 public:

  UeContextMme(int enbSocket, Log *log);
  void handleS1ApInitialUeMessage(S1ApInitialUeMessage message); 
  void handleS1ApInitialContextSetupResponse(S1ApInitialContextSetupResponse message); 
  void printState();

 private:

  int m_enbSocket;
  UeStateMme *m_state;

};

#endif
