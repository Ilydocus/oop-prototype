#include "UeContextEnb.hh"
#include <sys/socket.h>
#include <netdb.h>

#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"
#include <sstream>

using namespace std;

UeContextEnb::UeContextEnb(int ueSocket, int mmeSocket, Log *log):m_ueSocket(ueSocket),m_mmeSocket(mmeSocket),m_log(log){
  m_state = new UeStateEnb;
  m_state->rrcState=RRC_Idle;
  m_state->c_rnti=-1;
  (m_state->imsi).set_mcc("-1");
  (m_state->imsi).set_mnc("-1");
  (m_state->imsi).set_msin("-1");
  m_state->srbIdentity="-1";
  m_state->enbUeS1ApId=-1;
  for (int i = 0;i<NB_RAT;i++){
    (m_state->ratCapabilities[i]).set_rat(NO_RAT);
    (m_state->ratCapabilities[i]).set_issupported(false);
  }
  m_state->securityKey=-1;
  m_state->epsBearerId="-1";
}

void UeContextEnb::handleRaPreamble(RaPreamble message){
  ostringstream message_log;
  message_log << "Message received from Ue: RaPreamble {Rnti Type: " << message.ueidrntitype() << " Rnti Value: " << message.ueidrntivalue() << " }" << endl; 
  m_log->writeToLog(message_log.str());

  m_state->c_rnti = message.ueidrntivalue();

  RaResponse *raResponse = new RaResponse;
  raResponse->set_ueidrntitype(RA_RNTI);
  raResponse->set_ueidrntivalue(message.ueidrntivalue());
  raResponse->set_ueidcrnti(message.ueidrntivalue());

  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaR);
  rrcMessage.set_allocated_messagerar(raResponse);

  sendRrcMessage(m_ueSocket,rrcMessage);
}

void UeContextEnb::handleRrcConnectionRequest(RrcConnectionRequest message){

  ostringstream message_log;
  message_log << "Message received from Ue: RrcConnectionRequest {Rnti Type: " << message.ueidrntitype() << " Rnti Value: " << message.ueidrntivalue() << " UE identity is : " << (message.ueidentity()).mcc()<<"-"<< (message.ueidentity()).mnc() << "-"<< (message.ueidentity()).msin() <<" }" << endl; 
  m_log->writeToLog(message_log.str());

  m_state->imsi = message.ueidentity();

  RrcMessage rrcMessage;
  bool reject;
  reject = (message.ueidrntivalue() % 30) == 0;
  if (reject) {
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.ueidrntivalue());
    rrcCReject->set_waitingtime((message.ueidrntivalue() % 15)+1);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);
    }
  else {
    string * srbId = new string;
    genRandId(srbId, 8);
    m_state->srbIdentity = *srbId;

    RrcConnectionSetup *rrcCS = new RrcConnectionSetup;
    rrcCS->set_ueidrntitype(C_RNTI);
    rrcCS->set_ueidrntivalue(message.ueidrntivalue());
    rrcCS->set_srbidentity(*srbId);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCS);
    rrcMessage.set_allocated_messagerrccs(rrcCS);
    }

  sendRrcMessage(m_ueSocket,rrcMessage);
}

void UeContextEnb::handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message){
  ostringstream message_log;
  message_log << "Message received from Ue: RrcConnectionSetupComplete {C-Rnti: " << message.uecrnti() << " PLMN identity: " << message.selectedplmnidentity() << " }" << endl; 
  m_log->writeToLog(message_log.str());

  m_state->rrcState = RRC_Connected;
  m_state->enbUeS1ApId = 17 * message.uecrnti();
  
  S1ApInitialUeMessage *initialUeMessage = new S1ApInitialUeMessage;
  initialUeMessage->set_enb_ue_s1ap_id(17 * message.uecrnti());
  initialUeMessage->set_epsattachtype(EpsAttach);
  Imsi_message *tempImsi = new Imsi_message();
  *tempImsi = m_state->imsi;
  initialUeMessage->set_allocated_identity(tempImsi);
  //delete tempImsi?
  S1Message s1Message;
  s1Message.set_messagetype(S1Message_MessageType_TypeS1ApIUeM);
  s1Message.set_allocated_messages1apiuem(initialUeMessage);

  sendS1Message(m_mmeSocket,s1Message);

  S1Message *response = new S1Message;
  int receiveSuccess = receiveS1Message(m_mmeSocket,response);
  if(receiveSuccess){
    S1ApInitialContextSetupRequest initialCSRequest;
    initialCSRequest = s1Message.messages1apicsrequest();

    m_state->securityKey = initialCSRequest.securitykey();
    m_state->epsBearerId = initialCSRequest.epsbearerid();

    string plainMessage = "ciphered";
    string encryptedMessage; 
    byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
    memset (key,m_state->securityKey,CryptoPP::AES::DEFAULT_KEYLENGTH);
    memset (iv,0x00,CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption (aesEncryption, iv);
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(encryptedMessage));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plainMessage.c_str()),plainMessage.length()+1);
    stfEncryptor.MessageEnd();
    
    SecurityModeCommand *securityMCommand = new SecurityModeCommand;
    securityMCommand->set_uecrnti(initialCSRequest.enb_ue_s1ap_id()/17);
    securityMCommand->set_message_security(encryptedMessage);
    
    RrcMessage rrcMessage;
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeSecurityMCommand);
    rrcMessage.set_allocated_messagesecuritymcommand(securityMCommand);
    
    sendRrcMessage(m_ueSocket,rrcMessage);
  }
}

void UeContextEnb::handleSecurityModeComplete(SecurityModeComplete message){
  ostringstream message_log;
  message_log << "Message received from Ue: SecurityModeComplete {C-Rnti: " << message.uecrnti() << " Security Mode Success: " << message.securitymodesuccess() << " }" << endl; 
  m_log->writeToLog(message_log.str());

  RrcMessage rrcMessage;
  if(message.securitymodesuccess()){
    UeCapabilityEnquiry *ueCE = new UeCapabilityEnquiry;
    ueCE->set_uecrnti(message.uecrnti());
    ueCE->add_uecapabilityrequest(E_UTRA);
    ueCE->add_uecapabilityrequest(UTRA);
    ueCE->add_uecapabilityrequest(GERAN_CS);
    ueCE->add_uecapabilityrequest(GERAN_PS);
    ueCE->add_uecapabilityrequest(CDMA2000);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeUeCE);
    rrcMessage.set_allocated_messageuece(ueCE);
    }
  else {
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.uecrnti());
    rrcCReject->set_waitingtime((message.uecrnti() % 15)+1);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);

    m_state->rrcState = RRC_Idle;
  }
  sendRrcMessage(m_ueSocket,rrcMessage);
}

void UeContextEnb::handleUeCapabilityInformation(UeCapabilityInformation message){
  ostringstream message_log;
  message_log << "Message received from Ue: UeCapabilityInformation {C-Rnti: " << message.uecrnti() << " Ue RAT capabilities: " << " }" << endl; //RAT Capability List
  m_log->writeToLog(message_log.str());

  RrcConnectionReconfiguration *rrcCReconfiguration = new RrcConnectionReconfiguration;
  rrcCReconfiguration->set_uecrnti(message.uecrnti());
  rrcCReconfiguration->set_epsradiobeareridentity(m_state->epsBearerId);

  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReconfiguration);
  rrcMessage.set_allocated_messagerrccreconfiguration(rrcCReconfiguration);
  
  sendRrcMessage(m_ueSocket,rrcMessage);
}

void UeContextEnb::handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message){
  ostringstream message_log;
  message_log << "Message received from Ue: RrcConnectionReconfigurationComplete {C-Rnti: " << message.uecrnti() << " EPS Radio Bearer Activated: " << message.epsradiobeareractivated() << " }" << endl; 
  m_log->writeToLog(message_log.str());
  
  if(message.epsradiobeareractivated()){
    S1Message s1Message;
    S1ApInitialContextSetupResponse *iCSResponse = new S1ApInitialContextSetupResponse;
    iCSResponse->set_enb_ue_s1ap_id(m_state->enbUeS1ApId);
    string *erabId = new string;
    genRandId(erabId,5);
    iCSResponse->set_allocated_erabid(erabId);

    s1Message.set_messagetype(S1Message_MessageType_TypeS1ApICSResponse);
    s1Message.set_allocated_messages1apicsresponse(iCSResponse);

    sendS1Message(m_mmeSocket,s1Message);
 
    RrcMessage rrcMessage;
    RrcConnectionAccept *rrcCA = new RrcConnectionAccept;
    rrcCA->set_uecrnti(message.uecrnti());
 
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCA);
    rrcMessage.set_allocated_messagerrcca(rrcCA);

    sendRrcMessage(m_ueSocket,rrcMessage);
    printState();
  }
  else {
    RrcMessage rrcMessage;
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.uecrnti());
    rrcCReject->set_waitingtime((message.uecrnti() % 15)+1);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);

    m_state->rrcState = RRC_Idle;

    sendRrcMessage(m_ueSocket,rrcMessage);
  }
}

void UeContextEnb::printState(){
  ostringstream state;
  state << "Context at the end: UeContextEnb {RRC state: " << m_state->rrcState << "}" << endl; 
  m_log->writeToLog(state.str());
}
