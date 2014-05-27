#include "UeContextMme.hh"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>

using namespace std;

UeContextMme::UeContextMme(int enbSocket,Log * log):m_enbSocket(enbSocket)
{
  m_state = new UeStateMme;
  m_state->mmeUeS1ApId=-1;
  m_state->securityKey_mme= -1;

  m_log = log;
} 

void UeContextMme::handleS1ApInitialUeMessage(S1ApInitialUeMessage message){
  ostringstream message_log;
  message_log << "Message received from ENodeB: S1ApInitialUeMessage {Enb Ue S1Ap Id: " << message.enb_ue_s1ap_id() << " EPS Attach Type: " << message.epsattachtype() << " UE identity is : " << (message.identity()).mcc()<<"-"<< (message.identity()).mnc() << "-"<< (message.identity()).msin()<< " }" << endl; 
  m_log->writeToLog(message_log.str());

  m_state->mmeUeS1ApId = message.enb_ue_s1ap_id() - 4;
  m_state->securityKey_mme = (message.enb_ue_s1ap_id() / 17 )*18;

  string * epsBearerId = new string;
  genRandId(epsBearerId, 8);

  S1ApInitialContextSetupRequest *initialCSRequest = new S1ApInitialContextSetupRequest;
  initialCSRequest->set_mme_ue_s1ap_id(m_state->mmeUeS1ApId);
  initialCSRequest->set_enb_ue_s1ap_id(message.enb_ue_s1ap_id());
  initialCSRequest->set_securitykey(m_state->securityKey_mme);
  initialCSRequest->set_epsbearerid(*epsBearerId);
  delete epsBearerId;
  S1Message s1Message;
  s1Message.set_messagetype(S1Message_MessageType_TypeS1ApICSRequest);
  s1Message.set_allocated_messages1apicsrequest(initialCSRequest);

  sendS1Message(m_enbSocket,s1Message);
}
 
void UeContextMme::handleS1ApInitialContextSetupResponse(S1ApInitialContextSetupResponse message){
  ostringstream message_log;
  message_log << "Message received from ENodeB: S1ApInitialContextResponse {Enb Ue S1Ap Id: " << message.enb_ue_s1ap_id() << " ERab Id: " << message.erabid() << " }" << endl; 
  m_log->writeToLog(message_log.str());
  
  printState();
}

void UeContextMme::printState(){
  ostringstream state;
  state << "Context at the end: UeContextMme {MME S1Ap Ue Id: " << m_state->mmeUeS1ApId << " Security Key: " << m_state->securityKey_mme <<"}" << endl; 
  m_log->writeToLog(state.str());
}
