#include "UeContext.h"
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

UeContext::UeContext(int ueSocket, int mmeSocket):m_ueSocket(ueSocket),m_mmeSocket(mmeSocket)
{
}

void UeContext::handleRaPreamble(RaPreamble message)
{
  //Print message
  cout << "Message RA Preamble received " << endl;
  cout << "Type of rnti is : " << message.ueidrntitype() << endl;
  cout << "Value of rnti is : " << message.ueidrntivalue() << endl;
  //Create response
  RaResponse *raResponse = new RaResponse;
  raResponse->set_ueidrntitype(RA_RNTI);
  raResponse->set_ueidrntivalue(message.ueidrntivalue());
  raResponse->set_ueidcrnti(message.ueidrntivalue());
  //Pack it into a RrcMessage
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaR);
  rrcMessage.set_allocated_messagerar(raResponse);
  //Serialize message
  string output_message;
  rrcMessage.SerializeToString(&output_message);
  //Modify state
  m_state.c_rnti = message.ueidrntivalue();
  cout << "State is : " << m_state.c_rnti << endl;
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "RA Response sent " << endl;
  
}

void UeContext::handleRrcConnectionRequest(RrcConnectionRequest message){

  //Print message
  cout << "Message RRC Connection Request received " << endl;
  cout << "Type of rnti is : " << message.ueidrntitype() << endl;
  cout << "Value of rnti is : " << message.ueidrntivalue() << endl;
  cout << "UE identity is : " << (message.ueidentity()).mcc()<<"-"<< (message.ueidentity()).mnc() << "-"<< (message.ueidentity()).msin() << endl;
  //store Imsi
  m_state.imsi = message.ueidentity();
  //Decide if reject or accept connection
  RrcMessage rrcMessage;
  bool reject;
  reject = (message.ueidrntivalue() % 30)==0;
  if (reject) {
    //Create response
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.ueidrntivalue());
    rrcCReject->set_waitingtime((message.ueidrntivalue() % 15)+1);
    //Pack it
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);
  
  }
  else {
    //Create SRB
    string * srbId = new string;
    genRandId(srbId, 8);
    //Create response
    RrcConnectionSetup *rrcCS = new RrcConnectionSetup;
    rrcCS->set_ueidrntitype(C_RNTI);
    rrcCS->set_ueidrntivalue(message.ueidrntivalue());
    rrcCS->set_srbidentity(*srbId);
    //Store SRB
    m_state.srbIdentity = *srbId;
    //Pack it
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCS);
    rrcMessage.set_allocated_messagerrccs(rrcCS);
  }
  //Serialize message
  string output_message;
  rrcMessage.SerializeToString(&output_message);
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "RRC Connection Setup or Reject sent " << endl;
  
}

void UeContext::handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message){

}

//void UeContext::handleS1ApInitialContextSetupRequest(S1ApInitialContextSetupRequest){
//}

void UeContext::handleSecurityModeComplete(SecurityModeComplete message){
}

void UeContext::handleUeCapabilityInformation(UeCapabilityInformation message){
}

void UeContext::handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message){
}
