#ifndef DEF_UECONTEXTUE
#define DEF_UECONTEXTUE

#include <string>
#include "RrcMessages.pb.h"
#include "UeContext.hh"

struct UeStateUe{
  Imsi_message imsi;
  std::string srbId;
  int securityKey;
};

class UeContextUe : public UeContext {
public:
  UeContextUe(int ueId,int enbSocket,Log *log);
  void sendRaPreamble ();
  void handleRaResponse ();
  bool handleRrcConnectionSetup ();
  void handleSecurityModeCommand ();  
  bool handleUeCapabilityEnquiry();
  void handleRrcConnectionReconfiguration();
  void printState();

private:
  int mEnbSocket;
  UeStateUe mState;
  int mUeId;

  void genImsi (Imsi_message *imsi);
  std::string printCapabilityRequest(UeCapabilityEnquiry message);

};

#endif
