#ifndef DEF_UECONTEXTENB
#define DEF_UECONTEXTENB

#include <iostream>
#include <string>
#include "RrcMessages.pb.h"
#include "S1Messages.pb.h"
#include "UeContext.hh"

using namespace std;

enum RrcState {RRC_Idle, RRC_Connected};

struct UeStateEnb{
  RrcState rrcState;
  int c_rnti;
  Imsi_message imsi;
  string srbIdentity;
  int enbUeS1ApId;
  RatCapability ratCapabilities[5];
  int securityKey;
  string epsBearerId; 
};

class UeContextEnb : public UeContext
{
 public:

  UeContextEnb(int ueSocket, int mmeSocket);
  void handleRaPreamble(RaPreamble message);
  void handleRrcConnectionRequest(RrcConnectionRequest message);
  void handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message);
  //void handleS1ApInitialContextSetupRequest(S1ApInitialContextSetupRequest message); 
  void handleSecurityModeComplete(SecurityModeComplete message);
  void handleUeCapabilityInformation(UeCapabilityInformation message);
  void handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message);

 private:

  int m_ueSocket;
  int m_mmeSocket;
  UeStateEnb m_state;

};




#endif
