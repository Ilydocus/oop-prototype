#include "UeContextUe.hh"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"
#include <sstream>

UeContextUe::UeContextUe(int ueId,int enbSocket,Log *log){
  mState = new UeStateUe; 
  mEnbSocket = enbSocket;
  mState->securityKey = ueId * 18;
  mState->srbId = "-1";
  mUeId = ueId;
  mLog = log;

  Imsi_message *tempImsi = new Imsi_message;
  genImsi(tempImsi);
  mState->imsi = *tempImsi;
  delete tempImsi;
}

UeContextUe::~UeContextUe(){
}

void UeContextUe::genImsi(Imsi_message *imsi){
  //All the imsi are Swedish ones
  std::string *randomIdMnc= new std::string;
  std::string *randomIdMsin= new std::string;
  genRandId(randomIdMnc,2);
  genRandId(randomIdMsin,10);

  std::string *tempMcc = new std::string("260");
  imsi->set_mcc(*tempMcc);
  imsi->set_mnc(*randomIdMnc);
  imsi->set_msin(*randomIdMsin);
  
  delete randomIdMnc;
  delete randomIdMsin;
  delete tempMcc;
}

void UeContextUe::sendRaPreamble (){
  RaPreamble *raPreamble = new RaPreamble;
  raPreamble->set_ueidrntitype(RA_RNTI);
  raPreamble->set_ueidrntivalue(mUeId);
  
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaP);
  rrcMessage.set_allocated_messagerap(raPreamble);

  sendRrcMessage(mEnbSocket,rrcMessage);
}

void UeContextUe::handleRaResponse (RaResponse message) {
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: RaResponse {Rnti Type: " << message.ueidrntitype() << " Ra-Rnti value: " << message.ueidrntivalue() << " C-Rnti: " << message.ueidcrnti() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());
    
  RrcConnectionRequest *rrcConnectionRequest = new RrcConnectionRequest;
  rrcConnectionRequest->set_ueidrntitype(C_RNTI);
  rrcConnectionRequest->set_ueidrntivalue(message.ueidrntivalue());
  Imsi_message *tempImsi = new Imsi_message(mState->imsi);
  rrcConnectionRequest->set_allocated_ueidentity(tempImsi);
  RrcMessage rrcMessageO;
  rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeRrcCRequest);
  rrcMessageO.set_allocated_messagerrccrequest(rrcConnectionRequest);
    
  sendRrcMessage(mEnbSocket,rrcMessageO);
}

void UeContextUe::handleRrcConnectionSetup (RrcConnectionSetup message) {
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: RrcConnectionSetup {Rnti Type: " << message.ueidrntitype() << " Rnti value: " << message.ueidrntivalue() << " Srb Identity: " << message.srbidentity() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  mState->srbId = message.srbidentity();

  RrcConnectionSetupComplete *rrcCSC = new RrcConnectionSetupComplete;
  rrcCSC->set_uecrnti(message.ueidrntivalue());
  rrcCSC->set_selectedplmnidentity((mState->imsi).mcc() + (mState->imsi).mnc());
  RrcMessage rrcMessageO;
  rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeRrcCSC);
  rrcMessageO.set_allocated_messagerrccsc(rrcCSC);
	
  sendRrcMessage(mEnbSocket,rrcMessageO);
}

void UeContextUe::handleSecurityModeCommand (SecurityModeCommand message) {
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: SecurityModeCommand {C-Rnti: " << message.uecrnti() << " Message_security: " << message.message_security() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  std::string decryptedMessage;
  std::string cryptedMessage = message.message_security();
  byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
  memset (key,mState->securityKey,CryptoPP::AES::DEFAULT_KEYLENGTH);
  memset (iv,0x00,CryptoPP::AES::BLOCKSIZE);
  CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption (aesDecryption, iv);
  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedMessage));
  stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cryptedMessage.c_str()),cryptedMessage.size());
  stfDecryptor.MessageEnd();

  bool securityModeSuccess;
  securityModeSuccess = decryptedMessage.compare("ciphered");

  SecurityModeComplete *securityMComplete = new SecurityModeComplete;
  securityMComplete->set_uecrnti(message.uecrnti());
  securityMComplete->set_securitymodesuccess(securityModeSuccess);

  RrcMessage rrcMessageO;
  rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeSecurityMComplete);
  rrcMessageO.set_allocated_messagesecuritymcomplete(securityMComplete);

  sendRrcMessage(mEnbSocket,rrcMessageO);
}

void UeContextUe::handleUeCapabilityEnquiry (UeCapabilityEnquiry message) {
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: UeCapabilityEnquiry {C-Rnti: " << message.uecrnti() << " CapabilityRequest: " << printCapabilityRequest(message) << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());

  UeCapabilityInformation *ueCI = new UeCapabilityInformation;
  ueCI->set_uecrnti(message.uecrnti());
  srand(message.uecrnti() *6);
  for (int i = 0; i < message.uecapabilityrequest_size(); i++){
    RatCapability *ratCap = ueCI->add_uecapabilityratlist();
    ratCap->set_rat(message.uecapabilityrequest(i));
    ratCap->set_issupported(rand() % 2);
  }
  RrcMessage rrcMessageO;
  rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeUeCI);
  rrcMessageO.set_allocated_messageueci(ueCI);

  sendRrcMessage(mEnbSocket,rrcMessageO);
}

void UeContextUe::handleRrcConnectionReconfiguration(RrcConnectionReconfiguration message){
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: RrcConnectionReconfiguration {C-Rnti: " << message.uecrnti() << " Eps radio bearer identity: " << message.epsradiobeareridentity() << " }" << std::endl;
  mLog->writeToLog(messageLog.str());
		    		    
  bool epsBearerActivated;
  std::string epsBearer = message.epsradiobeareridentity();
  int len = (int)epsBearer.length();
  if ((epsBearer[0]== epsBearer[len]) == '9'){ epsBearerActivated = false;}
  else {epsBearerActivated = true;}

  RrcConnectionReconfigurationComplete *rrcCRC = new RrcConnectionReconfigurationComplete;
  rrcCRC->set_uecrnti(message.uecrnti());
  rrcCRC->set_epsradiobeareractivated(epsBearerActivated);
 
  RrcMessage rrcMessageO;
  rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeRrcCRC);
  rrcMessageO.set_allocated_messagerrccrc(rrcCRC);

  sendRrcMessage(mEnbSocket,rrcMessageO);
}

void UeContextUe::handleRrcConnectionReject(RrcConnectionReject message){
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << message.uecrnti() << " Waiting time: " << message.waitingtime() << " }" << std::endl; 
  mLog->writeToLog(messageLog.str());
  printState();
  std::cout << "UE "<< mUeId << " switched off..." << std::endl;
  close(mEnbSocket);
}

void UeContextUe::handleRrcConnectionAccept(RrcConnectionAccept message){
  std::ostringstream messageLog;
  messageLog << "Message received from ENodeB: RrcConnectionAccept {C-Rnti: " << message.uecrnti() << " }" << std::endl;
  mLog->writeToLog(messageLog.str());
  printState();
  std::cout << "UE "<< mUeId << " switched off..." << std::endl;
  close(mEnbSocket);
}

void UeContextUe::printState(){
  std::ostringstream state;
  state << "Context at the end: UeContextUe {Imsi: " << (mState->imsi).mcc()<<"-"<< (mState->imsi).mnc() << "-"<< (mState->imsi).msin() << " Security key:" << mState->securityKey << " SRB Id: " << mState->srbId << "}" << std::endl; 
  mLog->writeToLog(state.str());
}

std::string UeContextUe::printCapabilityRequest(UeCapabilityEnquiry message){
  std::ostringstream request;
  for(int i = 0; i<message.uecapabilityrequest_size();i++){
    request << message.uecapabilityrequest(i) <<",";
  }
  return request.str();
}
