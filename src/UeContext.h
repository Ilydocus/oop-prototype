#ifndef DEF_UECONTEXT
#define DEF_UECONTEXT

#include <iostream>
#include <string>
#include "RrcMessages.pb.h"
#include "Identifiers.h"

using namespace std;

enum RrcState {RRC_Idle, RRC_Connected};

struct UeContext_enb{
  RrcState rrcState;
  int c_rnti;
  Imsi_message imsi;
  string srbIdentity;
  int enbUeS1ApId;
  RatCapability ratCapabilities[5];
  int securityKey;
  string epsBearerId; 
};


class UeContext 
{
 public:

  UeContext(int ueSocket, int mmeSocket);
  void handleRaPreamble(RaPreamble message);
  void handleRrcConnectionRequest(RrcConnectionRequest message);
  void handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message);
  //void handleS1ApInitialContextSetupRequest(S1ApInitialContextSetupRequest); 
  void handleSecurityModeComplete(SecurityModeComplete message);
  void handleUeCapabilityInformation(UeCapabilityInformation message);
  void handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message);

 private:

  int m_ueSocket;
  int m_mmeSocket;
  UeContext_enb m_state;//struct needed?

};


#endif
