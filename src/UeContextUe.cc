#include "UeContextUe.hh"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"

UeContextUe::UeContextUe(int ueId,int enbSocket){

  m_enbSocket = enbSocket;

  Imsi_message * temp_imsi = new Imsi_message;
  genImsi(temp_imsi);
  m_state.imsi = *temp_imsi;
  delete temp_imsi;

  m_state.securityKey = ueId * 18;

  m_ueId = ueId;
  
}

void UeContextUe::genImsi(Imsi_message *imsi){
  //All the imsi are Swedish ones
  string *randomId_mnc= new string;
  string *randomId_msin= new string;
  genRandId(randomId_mnc,2);
  genRandId(randomId_msin,10);
  //randomId = genRandId(12);
  string *temp_mcc = new string("260");//p-e pas besoin
  imsi->set_mcc(*temp_mcc);
  imsi->set_mnc(*randomId_mnc);//randomId.substr(0,2));
  imsi->set_msin(*randomId_msin);//randomId.substr(2,10));
}

void UeContextUe::sendRaPreamble (){
  RaPreamble *rapreamble = new RaPreamble;
  rapreamble->set_ueidrntitype(RA_RNTI);
  rapreamble->set_ueidrntivalue(m_ueId);
  std::cout << "Ra_rnti is : " << rapreamble->ueidrntivalue() << std::endl;
  //Pack it into a RrcMessage
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaP);
  rrcMessage.set_allocated_messagerap(rapreamble);
  //Serialize the message
  std::string message;
  rrcMessage.SerializeToString(&message);
  std::cout << "Serialization completed " << std::endl;
  //Conversion of the message
  const char * ser_message;
  ser_message = message.c_str();
  //Socket code
  
  //status ==-1 error
  int len;
  ssize_t bytes_sent;
  len = strlen(ser_message);

  std::cout << "Len from strlen: " << len << ", len from strting: "
       << message.length() << std::endl;

  bytes_sent = send (m_enbSocket, message.c_str(), 
		     message.length(), 0);

  std::cout << "Message RAPreamble sent" << std::endl;
  std::cout << "Bytes sent: "<<bytes_sent << std::endl; 
}

void UeContextUe::handleRaResponse () {
  //Part 1: Recieve the message
ssize_t bytes_recieved;
		     char incoming_data_buffer[1000];
		     bytes_recieved = recv(m_enbSocket, incoming_data_buffer,1000, 0);
		      // If no data arrives, the program will just wait here until some data arrives.
		     if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl ;}
		       //return;}
		     if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl ;}//return;}
		     std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
		     if (bytes_recieved != -1 && bytes_recieved != 0){
		     incoming_data_buffer[bytes_recieved] = '\0';
		     //std::cout << incoming_data_buffer << std::endl;

		     //The message is serialized, we need to deserialize it
		     GOOGLE_PROTOBUF_VERIFY_VERSION;
    
		     RrcMessage rrcMessage;
		     //const string str_message;
		     string str_message(incoming_data_buffer, bytes_recieved);
		     rrcMessage.ParseFromString(str_message);
		     std::cout << "Message deserialized " << endl;
		     RaResponse raResponse;
		       raResponse = rrcMessage.messagerar();
		     cout << "Type of rnti is : " << raResponse.ueidrntitype() << endl;
  cout << "Value of rnti is : " << raResponse.ueidrntivalue() << endl;

		     //Part 2: Send Response
RrcConnectionRequest *rrcConnectionRequest = new RrcConnectionRequest;
  rrcConnectionRequest->set_ueidrntitype(C_RNTI);
  rrcConnectionRequest->set_ueidrntivalue(raResponse.ueidrntivalue());
  Imsi_message *tempImsi = new Imsi_message(m_state.imsi);
  //tempImsi = ue_state->imsi;
  rrcConnectionRequest->set_allocated_ueidentity(tempImsi);
  //delete tempImsi;?
  std::cout << "C_rnti is : " << rrcConnectionRequest->ueidrntivalue() << std::endl;
  //Pack it into a RrcMessage
  RrcMessage rrcMessage_o;
  rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCRequest);
  rrcMessage_o.set_allocated_messagerrccrequest(rrcConnectionRequest);
  //Serialize the message
  std::string message;
  rrcMessage_o.SerializeToString(&message);
  std::cout << "Serialization completed " << std::endl;

  ssize_t bytes_sent;
  bytes_sent = send (m_enbSocket, message.c_str(), 
		     message.length(), 0);

  std::cout << "Message RrcConnectionRequest sent" << std::endl;
  std::cout << "Bytes sent: "<<bytes_sent << std::endl;
		     }
		     
}

bool UeContextUe::handleRrcConnectionSetup () {
  //Part 1: Recieve the message
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(m_enbSocket, incoming_data_buffer,1000, 0);
  // If no data arrives, the program will just wait here until some data arrives.
  if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl ;}
  if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl ;}
  std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
  if (bytes_recieved != -1 && bytes_recieved != 0){
    incoming_data_buffer[bytes_recieved] = '\0';
    //The message is serialized, we need to deserialize it
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    RrcMessage rrcMessage;
    string str_message(incoming_data_buffer, bytes_recieved);
    rrcMessage.ParseFromString(str_message);
    std::cout << "Message deserialized " << endl;
    switch (rrcMessage.messagetype()) {
    case RrcMessage_MessageType_TypeRrcCS :
      {RrcConnectionSetup rrcConnectionSetup;
      rrcConnectionSetup = rrcMessage.messagerrccs();
      cout << "Setup: Value of rnti is : " << rrcConnectionSetup.ueidrntivalue() << endl;
      //add srb to the state
      m_state.srbId = rrcConnectionSetup.srbidentity();
      //Send Response
      RrcConnectionSetupComplete *rrcCSC = new RrcConnectionSetupComplete;
      rrcCSC->set_uecrnti(rrcConnectionSetup.ueidrntivalue());
      rrcCSC->set_selectedplmnidentity((m_state.imsi).mcc() + (m_state.imsi).mnc());
      //Pack it into a RrcMessage
      RrcMessage rrcMessage_o;
      rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCSC);
      rrcMessage_o.set_allocated_messagerrccsc(rrcCSC);
      //Serialize the message
      std::string message;
      rrcMessage_o.SerializeToString(&message);
      std::cout << "Serialization completed " << std::endl;

      ssize_t bytes_sent;
      bytes_sent = send (m_enbSocket, message.c_str(), 
			 message.length(), 0);

      std::cout << "Message RrcConnectionSetupComplete sent" << std::endl;
      std::cout << "Bytes sent: "<<bytes_sent << std::endl;

      //Not rejected
      return false;}
      break;
    case RrcMessage_MessageType_TypeRrcCReject :
      {RrcConnectionReject rrcConnectionReject;
      rrcConnectionReject = rrcMessage.messagerrccreject();
      cout << "Reject: Value of rnti is : " << rrcConnectionReject.uecrnti() << endl;
      //Rejected
      return true;}
      break;
    };
     }
		     
}

void UeContextUe::handleSecurityModeCommand () {
  //Part 1: Recieve the message
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(m_enbSocket, incoming_data_buffer,1000, 0);
  // If no data arrives, the program will just wait here until some data arrives.
  if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl ;}
  //return;}
  if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl ;}//return;}
  std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
  if (bytes_recieved != -1 && bytes_recieved != 0){
    incoming_data_buffer[bytes_recieved] = '\0';
    //std::cout << incoming_data_buffer << std::endl;
    
    //The message is serialized, we need to deserialize it
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    RrcMessage rrcMessage;
    //const string str_message;
    string str_message(incoming_data_buffer, bytes_recieved);
    rrcMessage.ParseFromString(str_message);
    std::cout << "Message deserialized " << endl;
    SecurityModeCommand securityMCommand;
    securityMCommand = rrcMessage.messagesecuritymcommand();
    cout << "Security M Command:  C-rnti is : " << securityMCommand.uecrnti() << endl;

    //Decrypt Message
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
    cout << "Decryption Result : " << decryptedMessage << endl;
    cout << "Decryption Result : " << securityModeSuccess << endl;

    //Part 2: Send Response
    SecurityModeComplete *securityMComplete = new SecurityModeComplete;
    securityMComplete->set_uecrnti(securityMCommand.uecrnti());
    securityMComplete->set_securitymodesuccess(securityModeSuccess);
    //Pack it into a RrcMessage
    RrcMessage rrcMessage_o;
    rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeSecurityMComplete);
    rrcMessage_o.set_allocated_messagesecuritymcomplete(securityMComplete);
    //Serialize the message
    std::string message;
    rrcMessage_o.SerializeToString(&message);
    std::cout << "Serialization completed " << std::endl;
      
    ssize_t bytes_sent;
    bytes_sent = send (m_enbSocket, message.c_str(), 
    message.length(), 0);
     
    std::cout << "Message SecurityModeComplete sent" << std::endl;
    std::cout << "Bytes sent: "<<bytes_sent << std::endl;
  }
  
}

bool UeContextUe::handleUeCapabilityEnquiry () {
  //Part 1: Recieve the message
  std::cout << " poufpouf2" << std::endl ;
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(m_enbSocket, incoming_data_buffer,1000, 0);
  // If no data arrives, the program will just wait here until some data arrives.
  if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl ;}
  //return;}
  if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl ;}//return;}
  std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
  if (bytes_recieved != -1 && bytes_recieved != 0){
    incoming_data_buffer[bytes_recieved] = '\0';
    //std::cout << incoming_data_buffer << std::endl;
    
    //The message is serialized, we need to deserialize it
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    RrcMessage rrcMessage;
    //const string str_message;
    string str_message(incoming_data_buffer, bytes_recieved);
    rrcMessage.ParseFromString(str_message);
    std::cout << "Message deserialized " << endl;
    switch (rrcMessage.messagetype()) {
    case RrcMessage_MessageType_TypeUeCE :
      {UeCapabilityEnquiry ueCE;
      ueCE = rrcMessage.messageuece();
      cout << "Capability Enquiry:  C-rnti is : " << ueCE.uecrnti() << endl;

      //Send Response
      UeCapabilityInformation *ueCI = new UeCapabilityInformation;
      ueCI->set_uecrnti(ueCE.uecrnti());
      //recup les differente sRAT
      srand(ueCE.uecrnti() *6);
      for (int i = 0; i < ueCE.uecapabilityrequest_size(); i++){
	RatCapability *ratCap = ueCI->add_uecapabilityratlist();
	ratCap->set_rat(ueCE.uecapabilityrequest(i));
	ratCap->set_issupported(rand() % 2);
	
      }
      //Pack it into a RrcMessage
      RrcMessage rrcMessage_o;
      rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeUeCI);
      rrcMessage_o.set_allocated_messageueci(ueCI);
      //Serialize the message
      std::string message;
      rrcMessage_o.SerializeToString(&message);
      std::cout << "Serialization completed " << std::endl;

      ssize_t bytes_sent;
      bytes_sent = send (m_enbSocket, message.c_str(), 
			 message.length(), 0);

      std::cout << "Message UeCI sent" << std::endl;
      std::cout << "Bytes sent: "<< bytes_sent << std::endl;

      //Not rejected
      return false;}
      break;
    case RrcMessage_MessageType_TypeRrcCReject :
      {RrcConnectionReject rrcConnectionReject;
      rrcConnectionReject = rrcMessage.messagerrccreject();
      cout << "Reject: Value of rnti is : " << rrcConnectionReject.uecrnti() << endl;
      //Rejected
      return true;}
      break;
    };
  }
  
}

 void UeContextUe::handleRrcConnectionReconfiguration(){
   //Part 1: Recieve the message
   ssize_t bytes_recieved;
   char incoming_data_buffer[1000];
		     bytes_recieved = recv(m_enbSocket, incoming_data_buffer,1000, 0);
		      // If no data arrives, the program will just wait here until some data arrives.
		     if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl ;}
		       //return;}
		     if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl ;}//return;}
		     std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
		     if (bytes_recieved != -1 && bytes_recieved != 0){
		     incoming_data_buffer[bytes_recieved] = '\0';
		     //std::cout << incoming_data_buffer << std::endl;

		     //The message is serialized, we need to deserialize it
		     GOOGLE_PROTOBUF_VERIFY_VERSION;
    
		     RrcMessage rrcMessage;
		     //const string str_message;
		     string str_message(incoming_data_buffer, bytes_recieved);
		     rrcMessage.ParseFromString(str_message);
		     std::cout << "Message deserialized " << endl;
		     RrcConnectionReconfiguration rrcCReconfiguration;
		       rrcCReconfiguration = rrcMessage.messagerrccreconfiguration();
		     cout << "RrcReconfiguration C-Rnti : " << rrcCReconfiguration.uecrnti() << endl;
 

		     //Part 2: Send Response
  /*RrcConnectionRequest *rrcConnectionRequest = new RrcConnectionRequest;
  rrcConnectionRequest->set_ueidrntitype(C_RNTI);
  rrcConnectionRequest->set_ueidrntivalue(raResponse.ueidrntivalue());
  Imsi_message *tempImsi = new Imsi_message(m_state.imsi);
  //tempImsi = ue_state->imsi;
  rrcConnectionRequest->set_allocated_ueidentity(tempImsi);
  //delete tempImsi;?
  std::cout << "C_rnti is : " << rrcConnectionRequest->ueidrntivalue() << std::endl;
  //Pack it into a RrcMessage
  RrcMessage rrcMessage_o;
  rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCRequest);
  rrcMessage_o.set_allocated_messagerrccrequest(rrcConnectionRequest);
  //Serialize the message
  std::string message;
  rrcMessage_o.SerializeToString(&message);
  std::cout << "Serialization completed " << std::endl;

  ssize_t bytes_sent;
  bytes_sent = send (m_enbSocket, message.c_str(), 
		     message.length(), 0);

  std::cout << "Message RrcConnectionReconfigurationComplete sent" << std::endl;
  std::cout << "Bytes sent: "<<bytes_sent << std::endl;*/
		     }
		     

}
