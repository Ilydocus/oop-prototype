#ifndef DEF_UECONTEXTUE
#define DEF_UECONTEXTUE

#include <string>
#include "RrcMessages.pb.h"
#include "UeContext.hh"

using namespace std;

struct UeStateUe{
  Imsi_message imsi;
  string srbId;
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
  int m_enbSocket;
  UeStateUe m_state;
  int m_ueId;

  void genImsi (Imsi_message *imsi);
  string printCapabilityRequest(UeCapabilityEnquiry message);

};

#endif
