#include "UeContextEnb.hh"
#include <sys/socket.h>
#include <netdb.h>
#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"
#include <sstream>

UeContextEnb::UeContextEnb(int ueSocket, int mmeSocket, Log *log):mUeSocket(ueSocket),mMmeSocket(mmeSocket){
  mState = new UeStateEnb;
  mState->rrcState=RRC_Idle;
  mState->cRnti=-1;
  (mState->imsi).set_mcc("-1");
  (mState->imsi).set_mnc("-1");
  (mState->imsi).set_msin("-1");
  mState->srbIdentity="-1";
  mState->enbUeS1ApId=-1;
  for (int i = 0;i<NB_RAT;i++){
    (mState->ratCapabilities[i]).set_rat(NO_RAT);
    (mState->ratCapabilities[i]).set_issupported(false);
  }
  mState->securityKey=-1;
  mState->epsBearerId="-1";

  mLog = log;
}

UeContextEnb::~UeContextEnb(){
  delete mState;
}

void UeContextEnb::handleRaPreamble(RaPreamble message){
  std::ostringstream messageLog;
  messageLog << "Message received from Ue: RaPreamble {Rnti Type: " << message.ueidrntitype() << " Rnti Value: " << message.ueidrntivalue() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  mState->cRnti = message.ueidrntivalue();

  RaResponse *raResponse = new RaResponse;
  raResponse->set_ueidrntitype(RA_RNTI);
  raResponse->set_ueidrntivalue(message.ueidrntivalue());
  raResponse->set_ueidcrnti(message.ueidrntivalue());

  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaR);
  rrcMessage.set_allocated_messagerar(raResponse);

  sendRrcMessage(mUeSocket,rrcMessage);
}

void UeContextEnb::handleRrcConnectionRequest(RrcConnectionRequest message){

  std::ostringstream messageLog;
  messageLog << "Message received from Ue: RrcConnectionRequest {Rnti Type: " << message.ueidrntitype() << " Rnti Value: " << message.ueidrntivalue() << " UE identity is : " << (message.ueidentity()).mcc()<<"-"<< (message.ueidentity()).mnc() << "-"<< (message.ueidentity()).msin() <<" }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  mState->imsi = message.ueidentity();

  RrcMessage rrcMessage;
  bool reject;
  reject = (message.ueidrntivalue() % 30) == 0;
  if (reject) {
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.ueidrntivalue());
    rrcCReject->set_waitingtime((message.ueidrntivalue() % 15)+1);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);
    printState();
    }
  else {
    std::string * srbId = new std::string;
    genRandId(srbId, 8);
    mState->srbIdentity = *srbId;
    delete srbId;

    RrcConnectionSetup *rrcCS = new RrcConnectionSetup;
    rrcCS->set_ueidrntitype(C_RNTI);
    rrcCS->set_ueidrntivalue(message.ueidrntivalue());
    rrcCS->set_srbidentity(*srbId);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCS);
    rrcMessage.set_allocated_messagerrccs(rrcCS);
    }

  sendRrcMessage(mUeSocket,rrcMessage);
}

void UeContextEnb::handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message){
  std::ostringstream messageLog;
  messageLog << "Message received from Ue: RrcConnectionSetupComplete {C-Rnti: " << message.uecrnti() << " PLMN identity: " << message.selectedplmnidentity() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  mState->rrcState = RRC_Connected;
  mState->enbUeS1ApId = 17 * message.uecrnti();
  
  S1ApInitialUeMessage *initialUeMessage = new S1ApInitialUeMessage;
  initialUeMessage->set_enb_ue_s1ap_id(17 * message.uecrnti());
  initialUeMessage->set_epsattachtype(EpsAttach);
  Imsi_message *tempImsi = new Imsi_message();
  *tempImsi = mState->imsi;
  initialUeMessage->set_allocated_identity(tempImsi);

  S1Message s1Message;
  s1Message.set_messagetype(S1Message_MessageType_TypeS1ApIUeM);
  s1Message.set_allocated_messages1apiuem(initialUeMessage);

  sendS1Message(mMmeSocket,s1Message);

  S1Message *response = new S1Message;
  int receiveSuccess = receiveS1Message(mMmeSocket,response);
  
  if(receiveSuccess){
    S1ApInitialContextSetupRequest initialCSRequest;
    initialCSRequest = response->messages1apicsrequest();

    mState->securityKey = initialCSRequest.securitykey();
    mState->epsBearerId = initialCSRequest.epsbearerid();

    std::string plainMessage = "ciphered";
    std::string encryptedMessage; 
    byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
    memset (key,mState->securityKey,CryptoPP::AES::DEFAULT_KEYLENGTH);
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
    
    sendRrcMessage(mUeSocket,rrcMessage);
  }
}

void UeContextEnb::handleSecurityModeComplete(SecurityModeComplete message){
  std::ostringstream messageLog;
  messageLog << "Message received from Ue: SecurityModeComplete {C-Rnti: " << message.uecrnti() << " Security Mode Success: " << message.securitymodesuccess() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

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

    mState->rrcState = RRC_Idle;
    printState();
  }
  sendRrcMessage(mUeSocket,rrcMessage);
}

void UeContextEnb::handleUeCapabilityInformation(UeCapabilityInformation message){
  std::ostringstream messageLog;
  messageLog << "Message received from Ue: UeCapabilityInformation {C-Rnti: " << message.uecrnti() << " Ue RAT capabilities: " << printRatCapabilities(message) <<" }" << std::endl;
  mLog->writeToLog(messageLog.str());
  
  if (message.uecapabilityratlist_size() != NB_RAT){
    std::cerr << "Incompatible numbers of RAT" << std::endl;
    return;
  }
  for(int i = 0; i<message.uecapabilityratlist_size();i++){
    (mState->ratCapabilities[i]).set_rat(message.uecapabilityratlist(i).rat());
    (mState->ratCapabilities[i]).set_issupported(message.uecapabilityratlist(i).issupported());
  }

  RrcConnectionReconfiguration *rrcCReconfiguration = new RrcConnectionReconfiguration;
  rrcCReconfiguration->set_uecrnti(message.uecrnti());
  rrcCReconfiguration->set_epsradiobeareridentity(mState->epsBearerId);

  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReconfiguration);
  rrcMessage.set_allocated_messagerrccreconfiguration(rrcCReconfiguration);
  
  sendRrcMessage(mUeSocket,rrcMessage);
}

void UeContextEnb::handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message){
  std::ostringstream messageLog;
  messageLog << "Message received from Ue: RrcConnectionReconfigurationComplete {C-Rnti: " << message.uecrnti() << " EPS Radio Bearer Activated: " << message.epsradiobeareractivated() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());
  
  if(message.epsradiobeareractivated()){
    S1Message s1Message;
    S1ApInitialContextSetupResponse *iCSResponse = new S1ApInitialContextSetupResponse;
    iCSResponse->set_enb_ue_s1ap_id(mState->enbUeS1ApId);
    std::string *erabId = new std::string;
    genRandId(erabId,5);
    iCSResponse->set_allocated_erabid(erabId);

    s1Message.set_messagetype(S1Message_MessageType_TypeS1ApICSResponse);
    s1Message.set_allocated_messages1apicsresponse(iCSResponse);

    sendS1Message(mMmeSocket,s1Message);
 
    RrcMessage rrcMessage;
    RrcConnectionAccept *rrcCA = new RrcConnectionAccept;
    rrcCA->set_uecrnti(message.uecrnti());
 
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCA);
    rrcMessage.set_allocated_messagerrcca(rrcCA);

    sendRrcMessage(mUeSocket,rrcMessage);
    printState();
  }
  else {
    RrcMessage rrcMessage;
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.uecrnti());
    rrcCReject->set_waitingtime((message.uecrnti() % 15)+1);

    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);

    mState->rrcState = RRC_Idle;

    sendRrcMessage(mUeSocket,rrcMessage);
    printState();
  }
}

void UeContextEnb::printState(){
  std::ostringstream state;
  state << "Context at the end: UeContextEnb {RRC state: " << mState->rrcState << " C-Rnti: " << mState->cRnti << " Imsi: " <<  (mState->imsi).mcc()<<"-"<< (mState->imsi).mnc() << "-"<< (mState->imsi).msin() << " SRB identity: "<<mState->srbIdentity << " EnB S1 Identity: " << mState->enbUeS1ApId << " RAT capabilities: " << printRatCapabilities(mState->ratCapabilities) << " security key: " << mState->securityKey << " EPS bearer id: " << mState->epsBearerId << "}" << std::endl; 
  mLog->writeToLog(state.str());
}

std::string UeContextEnb::printRatCapabilities(RatCapability *ratCapabilities){
  std::ostringstream rat;
  for(int i = 0; i<NB_RAT;i++){
    rat << "(" << ratCapabilities[i].rat() <<"," << ratCapabilities[i].issupported()<<")";
  }
  return rat.str();
}

std::string UeContextEnb::printRatCapabilities(UeCapabilityInformation message){
  std::ostringstream rat;
  for(int i = 0; i<message.uecapabilityratlist_size();i++){
    rat << "(" << message.uecapabilityratlist(i).rat() <<"," << message.uecapabilityratlist(i).issupported()<<")";
  }
  return rat.str();
}
