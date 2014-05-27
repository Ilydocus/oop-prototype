#include "UeContextUe.hh"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"
#include <sstream>

UeContextUe::UeContextUe(int ueId,int enbSocket,Log *log){

  m_enbSocket = enbSocket;
  m_state.securityKey = ueId * 18;
  m_ueId = ueId;
  m_log = log;

  Imsi_message *temp_imsi = new Imsi_message;
  genImsi(temp_imsi);
  m_state.imsi = *temp_imsi;
  delete temp_imsi;
}

void UeContextUe::genImsi(Imsi_message *imsi){
  //All the imsi are Swedish ones
  string *randomId_mnc= new string;
  string *randomId_msin= new string;
  genRandId(randomId_mnc,2);
  genRandId(randomId_msin,10);

  string *temp_mcc = new string("260");
  imsi->set_mcc(*temp_mcc);
  imsi->set_mnc(*randomId_mnc);
  imsi->set_msin(*randomId_msin);
  
  delete randomId_mnc;
  delete randomId_msin;
  delete temp_mcc;
}

void UeContextUe::sendRaPreamble (){
  RaPreamble *raPreamble = new RaPreamble;
  raPreamble->set_ueidrntitype(RA_RNTI);
  raPreamble->set_ueidrntivalue(m_ueId);
  
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaP);
  rrcMessage.set_allocated_messagerap(raPreamble);

  sendRrcMessage(m_enbSocket,rrcMessage);
}

void UeContextUe::handleRaResponse () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(m_enbSocket,message);
  if(receiveSuccess){
    if (message->messagetype() != RrcMessage_MessageType_TypeRaR) cerr << "Invalid message type, should be RaResponse" << endl;
    RaResponse raResponse;
    raResponse = message->messagerar();
    delete message;
    ostringstream message_log;
    message_log << "Message received from ENodeB: RaResponse {Rnti Type: " << raResponse.ueidrntitype() << " Ra-Rnti value: " << raResponse.ueidrntivalue() << " C-Rnti: " << raResponse.ueidcrnti() << " }" << endl; 
    m_log->writeToLog(message_log.str());
    
    RrcConnectionRequest *rrcConnectionRequest = new RrcConnectionRequest;
    rrcConnectionRequest->set_ueidrntitype(C_RNTI);
    rrcConnectionRequest->set_ueidrntivalue(raResponse.ueidrntivalue());
    Imsi_message *tempImsi = new Imsi_message(m_state.imsi);
    rrcConnectionRequest->set_allocated_ueidentity(tempImsi);
    RrcMessage rrcMessage_o;
    rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCRequest);
    rrcMessage_o.set_allocated_messagerrccrequest(rrcConnectionRequest);
    
    sendRrcMessage(m_enbSocket,rrcMessage_o);
  }
}

bool UeContextUe::handleRrcConnectionSetup () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(m_enbSocket,message);
  if(receiveSuccess){
    switch (message->messagetype()) {
      case RrcMessage_MessageType_TypeRrcCS :
	{RrcConnectionSetup rrcConnectionSetup;
	rrcConnectionSetup = message->messagerrccs();
	delete message;
	ostringstream message_log;
	message_log << "Message received from ENodeB: RrcConnectionSetup {Rnti Type: " << rrcConnectionSetup.ueidrntitype() << " Rnti value: " << rrcConnectionSetup.ueidrntivalue() << " Srb Identity: " << rrcConnectionSetup.srbidentity() << " }" << endl; 
	m_log->writeToLog(message_log.str());

	m_state.srbId = rrcConnectionSetup.srbidentity();

	RrcConnectionSetupComplete *rrcCSC = new RrcConnectionSetupComplete;
	rrcCSC->set_uecrnti(rrcConnectionSetup.ueidrntivalue());
	rrcCSC->set_selectedplmnidentity((m_state.imsi).mcc() + (m_state.imsi).mnc());
	RrcMessage rrcMessage_o;
	rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCSC);
	rrcMessage_o.set_allocated_messagerrccsc(rrcCSC);
	
	sendRrcMessage(m_enbSocket,rrcMessage_o);
	return false;}
	break;
      case RrcMessage_MessageType_TypeRrcCReject :
	{RrcConnectionReject rrcConnectionReject;
	rrcConnectionReject = message->messagerrccreject();
	delete message;
	ostringstream message_log;
	message_log << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << rrcConnectionReject.uecrnti() << " Waiting time: " << rrcConnectionReject.waitingtime() << " }" << endl; 
	m_log->writeToLog(message_log.str());
	printState();
	return true;}
	break;
      default:
	delete message;
	cerr << "Invalid message type, should be RrcConnectionSetup or RrcConnectionReject" << endl;
  };
  }
}

void UeContextUe::handleSecurityModeCommand () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(m_enbSocket,message);
  if(receiveSuccess){
    if (message->messagetype() != RrcMessage_MessageType_TypeSecurityMCommand) cerr << "Invalid message type, should be SecurityModeCommand" << endl;
    SecurityModeCommand securityMCommand;
    securityMCommand = message->messagesecuritymcommand();
    delete message;
    ostringstream message_log;
    message_log << "Message received from ENodeB: SecurityModeCommand {C-Rnti: " << securityMCommand.uecrnti() << " Message_security: " << securityMCommand.message_security() << " }" << endl; 
    m_log->writeToLog(message_log.str());

    string decryptedMessage;
    string cryptedMessage = securityMCommand.message_security();
    byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
    memset (key,m_state.securityKey,CryptoPP::AES::DEFAULT_KEYLENGTH);
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

    RrcMessage rrcMessage_o;
    rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeSecurityMComplete);
    rrcMessage_o.set_allocated_messagesecuritymcomplete(securityMComplete);

    sendRrcMessage(m_enbSocket,rrcMessage_o);
  }
}

bool UeContextUe::handleUeCapabilityEnquiry () {
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(m_enbSocket,message);
  if(receiveSuccess){
    switch (message->messagetype()) {
      case RrcMessage_MessageType_TypeUeCE :
	{UeCapabilityEnquiry ueCE;
	ueCE = message->messageuece();
	delete message;
	ostringstream message_log;
	message_log << "Message received from ENodeB: UeCapabilityEnquiry {C-Rnti: " << ueCE.uecrnti() << " CapabilityRequest: " << printCapabilityRequest(ueCE) << " }" << endl; 
	m_log->writeToLog(message_log.str());

	UeCapabilityInformation *ueCI = new UeCapabilityInformation;
	ueCI->set_uecrnti(ueCE.uecrnti());
	srand(ueCE.uecrnti() *6);
	for (int i = 0; i < ueCE.uecapabilityrequest_size(); i++){
	  RatCapability *ratCap = ueCI->add_uecapabilityratlist();
	  ratCap->set_rat(ueCE.uecapabilityrequest(i));
	  ratCap->set_issupported(rand() % 2);
	}
	RrcMessage rrcMessage_o;
	rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeUeCI);
	rrcMessage_o.set_allocated_messageueci(ueCI);

	sendRrcMessage(m_enbSocket,rrcMessage_o);
	return false;}
	break;
      case RrcMessage_MessageType_TypeRrcCReject :
	{RrcConnectionReject rrcConnectionReject;
	rrcConnectionReject = message->messagerrccreject();
	delete message;
	ostringstream message_log;
	message_log << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << rrcConnectionReject.uecrnti() << " Waiting time: " << rrcConnectionReject.waitingtime() << " }" << endl; 
	m_log->writeToLog(message_log.str());
	printState();
	return true;}
	break;
      default:
	delete message;
	cerr << "Invalid message type, should be UeCapabilityEnquiry or RrcConnectionReject" << endl;
    };
  }
}

void UeContextUe::handleRrcConnectionReconfiguration(){
  RrcMessage *message = new RrcMessage;
  int receiveSuccess = receiveRrcMessage(m_enbSocket,message);
  if(receiveSuccess){
    if (message->messagetype() != RrcMessage_MessageType_TypeRrcCReconfiguration) cerr << "Invalid message type, should be RrcConnectionReconfiguration" << endl;
    RrcConnectionReconfiguration rrcCReconfiguration;
    rrcCReconfiguration = message->messagerrccreconfiguration();
    delete message;
    ostringstream message_log;
    message_log << "Message received from ENodeB: RrcConnectionReconfiguration {C-Rnti: " << rrcCReconfiguration.uecrnti() << " Eps radio bearer identity: " << rrcCReconfiguration.epsradiobeareridentity() << " }" << endl;
    m_log->writeToLog(message_log.str());
		    		    
    bool epsBearerActivated;
    string epsBearer = rrcCReconfiguration.epsradiobeareridentity();
    int len = (int)epsBearer.length();
    if ((epsBearer[0]== epsBearer[len]) == '9'){ epsBearerActivated = false;}
    else {epsBearerActivated = true;}

    RrcConnectionReconfigurationComplete *rrcCRC = new RrcConnectionReconfigurationComplete;
    rrcCRC->set_uecrnti(rrcCReconfiguration.uecrnti());
    rrcCRC->set_epsradiobeareractivated(epsBearerActivated);
 
    RrcMessage rrcMessage_o;
    rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCRC);
    rrcMessage_o.set_allocated_messagerrccrc(rrcCRC);

    sendRrcMessage(m_enbSocket,rrcMessage_o);
    
    RrcMessage *message2 = new RrcMessage;
    int receiveSuccess2 = receiveRrcMessage(m_enbSocket,message2);
    if(receiveSuccess2){
      switch (message2->messagetype()){
        case RrcMessage_MessageType_TypeRrcCA :
          {RrcConnectionAccept rrcCA;
	  rrcCA = message2->messagerrcca();
	  delete message2;
	  message_log << "Message received from ENodeB: RrcConnectionAccept {C-Rnti: " << rrcCA.uecrnti() << " }" << endl;
	  m_log->writeToLog(message_log.str());
	  printState();}
	  break;
        case RrcMessage_MessageType_TypeRrcCReject :
	  {RrcConnectionReject rrcConnectionReject;
	  rrcConnectionReject = message2->messagerrccreject();
	  delete message2;
	  message_log << "Message received from ENodeB: RrcConnectionReject {C-Rnti: " << rrcConnectionReject.uecrnti() << " Waiting time: " << rrcConnectionReject.waitingtime() << " }" << endl; 
	  m_log->writeToLog(message_log.str());
	  printState();}
	  break;
        default:
	  cerr << "Invalid message type, should be RrcConnectionAccept or RrcConnectionReject" << endl;
	  delete message2;
      };
    }
  }
}

void UeContextUe::printState(){
  ostringstream state;
  state << "Context at the end: UeContextUe {Imsi: " << (m_state.imsi).mcc()<<"-"<< (m_state.imsi).mnc() << "-"<< (m_state.imsi).msin() << " Security key:" << m_state.securityKey << "}" << endl; 
  m_log->writeToLog(state.str());
}

string UeContextUe::printCapabilityRequest(UeCapabilityEnquiry message){
  ostringstream request;
  for(int i = 0; i<message.uecapabilityrequest_size();i++){
    request << message.uecapabilityrequest(i) <<",";
  }
  return request.str();
}
