#include "UeContextUe.hh"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"
#include <sstream>

UeContextUe::UeContextUe(int ueId,int enbSocket,Log *log){

  mEnbSocket = enbSocket;
  mState.securityKey = ueId * 18;
  mUeId = ueId;
  mLog = log;

  Imsi_message *tempImsi = new Imsi_message;
  genImsi(tempImsi);
  mState.imsi = *tempImsi;
  delete tempImsi;
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

void UeContextUe::handleRaResponse () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(mEnbSocket,message);
  if(receiveSuccess){
    if (message->messagetype() != RrcMessage_MessageType_TypeRaR) std::cerr << "Invalid message type, should be RaResponse" << std::endl;
    RaResponse raResponse;
    raResponse = message->messagerar();
    delete message;
    std::ostringstream messageLog;
    messageLog << "Message received from ENodeB: RaResponse {Rnti Type: " << raResponse.ueidrntitype() << " Ra-Rnti value: " << raResponse.ueidrntivalue() << " C-Rnti: " << raResponse.ueidcrnti() << " }" << std::endl; 
    mLog->writeToLog(messageLog.str());
    
    RrcConnectionRequest *rrcConnectionRequest = new RrcConnectionRequest;
    rrcConnectionRequest->set_ueidrntitype(C_RNTI);
    rrcConnectionRequest->set_ueidrntivalue(raResponse.ueidrntivalue());
    Imsi_message *tempImsi = new Imsi_message(mState.imsi);
    rrcConnectionRequest->set_allocated_ueidentity(tempImsi);
    RrcMessage rrcMessageO;
    rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeRrcCRequest);
    rrcMessageO.set_allocated_messagerrccrequest(rrcConnectionRequest);
    
    sendRrcMessage(mEnbSocket,rrcMessageO);
  }
}

bool UeContextUe::handleRrcConnectionSetup () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(mEnbSocket,message);
  if(receiveSuccess){
    switch (message->messagetype()) {
      case RrcMessage_MessageType_TypeRrcCS :
	{RrcConnectionSetup rrcConnectionSetup;
	rrcConnectionSetup = message->messagerrccs();
	delete message;
	std::ostringstream messageLog;
	messageLog << "Message received from ENodeB: RrcConnectionSetup {Rnti Type: " << rrcConnectionSetup.ueidrntitype() << " Rnti value: " << rrcConnectionSetup.ueidrntivalue() << " Srb Identity: " << rrcConnectionSetup.srbidentity() << " }" << std::endl; 
	mLog->writeToLog(messageLog.str());

	mState.srbId = rrcConnectionSetup.srbidentity();

	RrcConnectionSetupComplete *rrcCSC = new RrcConnectionSetupComplete;
	rrcCSC->set_uecrnti(rrcConnectionSetup.ueidrntivalue());
	rrcCSC->set_selectedplmnidentity((mState.imsi).mcc() + (mState.imsi).mnc());
	RrcMessage rrcMessageO;
	rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeRrcCSC);
	rrcMessageO.set_allocated_messagerrccsc(rrcCSC);
	
	sendRrcMessage(mEnbSocket,rrcMessageO);
	return false;}
	break;
      case RrcMessage_MessageType_TypeRrcCReject :
	{RrcConnectionReject rrcConnectionReject;
	rrcConnectionReject = message->messagerrccreject();
	delete message;
	std::ostringstream messageLog;
	messageLog << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << rrcConnectionReject.uecrnti() << " Waiting time: " << rrcConnectionReject.waitingtime() << " }" << std::endl; 
	mLog->writeToLog(messageLog.str());
	printState();
	return true;}
	break;
      default:
	delete message;
	std::cerr << "Invalid message type, should be RrcConnectionSetup or RrcConnectionReject" << std::endl;
  };
  }
}

void UeContextUe::handleSecurityModeCommand () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(mEnbSocket,message);
  if(receiveSuccess){
    if (message->messagetype() != RrcMessage_MessageType_TypeSecurityMCommand) std::cerr << "Invalid message type, should be SecurityModeCommand" << std::endl;
    SecurityModeCommand securityMCommand;
    securityMCommand = message->messagesecuritymcommand();
    delete message;
    std::ostringstream messageLog;
    messageLog << "Message received from ENodeB: SecurityModeCommand {C-Rnti: " << securityMCommand.uecrnti() << " Message_security: " << securityMCommand.message_security() << " }" << std::endl; 
    mLog->writeToLog(messageLog.str());

    std::string decryptedMessage;
    std::string cryptedMessage = securityMCommand.message_security();
    byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
    memset (key,mState.securityKey,CryptoPP::AES::DEFAULT_KEYLENGTH);
    memset (iv,0x00,CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption (aesDecryption, iv);
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedMessage));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cryptedMessage.c_str()),cryptedMessage.size());
    stfDecryptor.MessageEnd();

    bool securityModeSuccess;
    securityModeSuccess = decryptedMessage.compare("ciphered");

    SecurityModeComplete *securityMComplete = new SecurityModeComplete;
    securityMComplete->set_uecrnti(securityMCommand.uecrnti());
    securityMComplete->set_securitymodesuccess(securityModeSuccess);

    RrcMessage rrcMessageO;
    rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeSecurityMComplete);
    rrcMessageO.set_allocated_messagesecuritymcomplete(securityMComplete);

    sendRrcMessage(mEnbSocket,rrcMessageO);
  }
}

bool UeContextUe::handleUeCapabilityEnquiry () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(mEnbSocket,message);
  if(receiveSuccess){
    switch (message->messagetype()) {
      case RrcMessage_MessageType_TypeUeCE :
	{UeCapabilityEnquiry ueCE;
	ueCE = message->messageuece();
	delete message;
	std::ostringstream messageLog;
	messageLog << "Message received from ENodeB: UeCapabilityEnquiry {C-Rnti: " << ueCE.uecrnti() << " CapabilityRequest: " << printCapabilityRequest(ueCE) << " }" << std::endl; 
	mLog->writeToLog(messageLog.str());

	UeCapabilityInformation *ueCI = new UeCapabilityInformation;
	ueCI->set_uecrnti(ueCE.uecrnti());
	srand(ueCE.uecrnti() *6);
	for (int i = 0; i < ueCE.uecapabilityrequest_size(); i++){
	  RatCapability *ratCap = ueCI->add_uecapabilityratlist();
	  ratCap->set_rat(ueCE.uecapabilityrequest(i));
	  ratCap->set_issupported(rand() % 2);
	}
	RrcMessage rrcMessageO;
	rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeUeCI);
	rrcMessageO.set_allocated_messageueci(ueCI);

	sendRrcMessage(mEnbSocket,rrcMessageO);
	return false;}
	break;
      case RrcMessage_MessageType_TypeRrcCReject :
	{RrcConnectionReject rrcConnectionReject;
	rrcConnectionReject = message->messagerrccreject();
	delete message;
	std::ostringstream messageLog;
	messageLog << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << rrcConnectionReject.uecrnti() << " Waiting time: " << rrcConnectionReject.waitingtime() << " }" << std::endl; 
	mLog->writeToLog(messageLog.str());
	printState();
	return true;}
	break;
      default:
	delete message;
	std::cerr << "Invalid message type, should be UeCapabilityEnquiry or RrcConnectionReject" << std::endl;
    };
  }
}

void UeContextUe::handleRrcConnectionReconfiguration(){
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(mEnbSocket,message);
  if(receiveSuccess){
    if (message->messagetype() != RrcMessage_MessageType_TypeRrcCReconfiguration) std::cerr << "Invalid message type, should be RrcConnectionReconfiguration" << std::endl;
    RrcConnectionReconfiguration rrcCReconfiguration;
    rrcCReconfiguration = message->messagerrccreconfiguration();
    delete message;
    std::ostringstream messageLog;
    messageLog << "Message received from ENodeB: RrcConnectionReconfiguration {C-Rnti: " << rrcCReconfiguration.uecrnti() << " Eps radio bearer identity: " << rrcCReconfiguration.epsradiobeareridentity() << " }" << std::endl;
    mLog->writeToLog(messageLog.str());
		    		    
    bool epsBearerActivated;
    std::string epsBearer = rrcCReconfiguration.epsradiobeareridentity();
    int len = (int)epsBearer.length();
    if ((epsBearer[0]== epsBearer[len]) == '9'){ epsBearerActivated = false;}
    else {epsBearerActivated = true;}

    RrcConnectionReconfigurationComplete *rrcCRC = new RrcConnectionReconfigurationComplete;
    rrcCRC->set_uecrnti(rrcCReconfiguration.uecrnti());
    rrcCRC->set_epsradiobeareractivated(epsBearerActivated);
 
    RrcMessage rrcMessageO;
    rrcMessageO.set_messagetype(RrcMessage_MessageType_TypeRrcCRC);
    rrcMessageO.set_allocated_messagerrccrc(rrcCRC);

    sendRrcMessage(mEnbSocket,rrcMessageO);
    
    RrcMessage *message2 = new RrcMessage;
    int receiveSuccess2 = receiveRrcMessage(mEnbSocket,message2);
    if(receiveSuccess2){
      switch (message2->messagetype()){
        case RrcMessage_MessageType_TypeRrcCA :
          {RrcConnectionAccept rrcCA;
	  rrcCA = message2->messagerrcca();
	  delete message2;
	  messageLog << "Message received from ENodeB: RrcConnectionAccept {C-Rnti: " << rrcCA.uecrnti() << " }" << std::endl;
	  mLog->writeToLog(messageLog.str());
	  printState();}
	  break;
        case RrcMessage_MessageType_TypeRrcCReject :
	  {RrcConnectionReject rrcConnectionReject;
	  rrcConnectionReject = message2->messagerrccreject();
	  delete message2;
	  messageLog << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << rrcConnectionReject.uecrnti() << " Waiting time: " << rrcConnectionReject.waitingtime() << " }" << std::endl; 
	  mLog->writeToLog(messageLog.str());
	  printState();}
	  break;
        default:
	  std::cerr << "Invalid message type, should be RrcConnectionAccept or RrcConnectionReject" << std::endl;
	  delete message2;
      };
    }
  }
}

void UeContextUe::printState(){
  std::ostringstream state;
  state << "Context at the end: UeContextUe {Imsi: " << (mState.imsi).mcc()<<"-"<< (mState.imsi).mnc() << "-"<< (mState.imsi).msin() << " Security key:" << mState.securityKey << " SRB Id: " << mState.srbId << "}" << std::endl; 
  mLog->writeToLog(state.str());
}

std::string UeContextUe::printCapabilityRequest(UeCapabilityEnquiry message){
  std::ostringstream request;
  for(int i = 0; i<message.uecapabilityrequest_size();i++){
    request << message.uecapabilityrequest(i) <<",";
  }
  return request.str();
}
