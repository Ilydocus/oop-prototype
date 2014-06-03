#ifndef DEF_UECONTEXTENB
#define DEF_UECONTEXTENB

#include <iostream>
#include <string>
#include "RrcMessages.pb.h"
#include "S1Messages.pb.h"
#include "UeContext.hh"

#define NB_RAT 5

enum RrcState {RRC_Idle, RRC_Connected};

struct UeStateEnb{
  RrcState rrcState;
  int cRnti;
  Imsi_message imsi;
  std::string srbIdentity;
  int enbUeS1ApId;
  RatCapability ratCapabilities[NB_RAT];
  int securityKey;
  std::string epsBearerId; 
};

class UeContextEnb : public UeContext{
public:
  UeContextEnb(int ueSocket, int mmeSocket, Log* log);
  ~UeContextEnb();
  void handleRaPreamble(RaPreamble message);
  void handleRrcConnectionRequest(RrcConnectionRequest message);
  void handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message);
  void handleS1ApInitialContextSetupRequest(S1ApInitialContextSetupRequest message); 
  void handleSecurityModeComplete(SecurityModeComplete message);
  void handleUeCapabilityInformation(UeCapabilityInformation message);
  void handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message);
  void printState();

private:
  int mUeSocket;
  int mMmeSocket;
  UeStateEnb *mState;

  std::string printRatCapabilities(RatCapability *ratCapabilities);
  std::string printRatCapabilities(UeCapabilityInformation message);
};

#endif
