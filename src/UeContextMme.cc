#include "UeContextMme.hh"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <sstream>
#include <sys/time.h>

UeContextMme::UeContextMme(int enbSocket,Log * log):mEnbSocket(enbSocket)
{
  mState = new UeStateMme;
  mState->mmeUeS1ApId=-1;
  mState->securityKeyMme= -1;

  mLog = log;
} 

UeContextMme::~UeContextMme(){
  delete mState;
}

void UeContextMme::handleS1ApInitialUeMessage(S1ApInitialUeMessage message){
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: S1ApInitialUeMessage {Enb Ue S1Ap Id: " << message.enb_ue_s1ap_id() << " EPS Attach Type: " << message.epsattachtype() << " UE identity is : " << (message.identity()).mcc()<<"-"<< (message.identity()).mnc() << "-"<< (message.identity()).msin()<< " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  mState->mmeUeS1ApId = message.enb_ue_s1ap_id() - 4;
  mState->securityKeyMme = (message.enb_ue_s1ap_id() / 17 )*18;

  std::string * epsBearerId = new std::string;
  genRandId(epsBearerId, 8);

  S1ApInitialContextSetupRequest *initialCSRequest = new S1ApInitialContextSetupRequest;
  initialCSRequest->set_mme_ue_s1ap_id(mState->mmeUeS1ApId);
  initialCSRequest->set_enb_ue_s1ap_id(message.enb_ue_s1ap_id());
  initialCSRequest->set_securitykey(mState->securityKeyMme);
  initialCSRequest->set_epsbearerid(*epsBearerId);
  delete epsBearerId;
  S1Message s1Message;
  s1Message.set_messagetype(S1Message_MessageType_TypeS1ApICSRequest);
  s1Message.set_allocated_messages1apicsrequest(initialCSRequest);

  sendS1Message(mEnbSocket,s1Message);
}
 
void UeContextMme::handleS1ApInitialContextSetupResponse(S1ApInitialContextSetupResponse message){
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: S1ApInitialContextResponse {Enb Ue S1Ap Id: " << message.enb_ue_s1ap_id() << " ERab Id: " << message.erabid() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());
  printState();

  timeval endProcedure;
  std::ostringstream messageLog2;
  gettimeofday(&endProcedure, NULL);
  messageLog2 << "For Ue with C-Rnti " << message.enb_ue_s1ap_id()/17 << " : end of procedure " << endProcedure.tv_usec + endProcedure.tv_sec * 1000000 << std::endl;
  mLog->writeToLog(messageLog2.str());
}

void UeContextMme::printState(){
  std::ostringstream state;
  state << "Context at the end: UeContextMme {MME S1Ap Ue Id: " << mState->mmeUeS1ApId << " Security Key: " << mState->securityKeyMme <<"}" << std::endl; 
  mLog->writeToLog(state.str());
}
