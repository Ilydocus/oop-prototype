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
  ~UeContextUe();
  void sendRaPreamble ();
  void handleRaResponse (RaResponse message);
  void handleRrcConnectionSetup (RrcConnectionSetup message);
  void handleSecurityModeCommand (SecurityModeCommand message);  
  void handleUeCapabilityEnquiry(UeCapabilityEnquiry message);
  void handleRrcConnectionReconfiguration(RrcConnectionReconfiguration message);
  void handleRrcConnectionReject(RrcConnectionReject message);
  void handleRrcConnectionAccept(RrcConnectionAccept message);
  void printState();
 
private:
  int mEnbSocket;
  UeStateUe *mState;
  int mUeId;

  void genImsi (Imsi_message *imsi);
  std::string printCapabilityRequest(UeCapabilityEnquiry message);

};

#endif
