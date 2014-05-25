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
  //Initialize the state
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
  std::cout << "Initializing context"  << std::endl;
}

void UeContextEnb::handleRaPreamble(RaPreamble message)
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
  m_state->c_rnti = message.ueidrntivalue();
  cout << "State is : " << m_state->c_rnti << endl;
  //Send Response
  sendMessage(m_ueSocket,output_message);

//bytes_sent = send (m_ueSocket, output_message.c_str(), 
//		     output_message.length(), 0);
  cout << "RA Response sent " << endl;
  cout << "M_state crnti : "<<  m_state->c_rnti << endl;
  
}

void UeContextEnb::handleRrcConnectionRequest(RrcConnectionRequest message){

  //Print message
  cout << "M_state crnti : "<<  m_state->c_rnti << endl;
  cout << "Message RRC Connection Request received " << endl;
  cout << "Type of rnti is : " << message.ueidrntitype() << endl;
  cout << "Value of rnti is : " << message.ueidrntivalue() << endl;
  cout << "UE identity is : " << (message.ueidentity()).mcc()<<"-"<< (message.ueidentity()).mnc() << "-"<< (message.ueidentity()).msin() << endl;
  //store Imsi
  m_state->imsi = message.ueidentity();
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
    m_state->srbIdentity = *srbId;
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
  cout << "M_state crnti : "<<  m_state->c_rnti << endl;
}

void UeContextEnb::handleRrcConnectionSetupComplete(RrcConnectionSetupComplete message){

  //Print message
  cout << "Message RRC Connection Setup Complete received " << endl;
  cout << "Value of c-rnti is : " << message.uecrnti() << endl;
  cout << "Plmn identity is : " << message.selectedplmnidentity() << endl;
  //Change state
  m_state->rrcState = RRC_Connected;
  m_state->enbUeS1ApId = 17 * message.uecrnti();
  cout << "M_state enb : "<<  m_state->enbUeS1ApId << endl;
  
  //Send response to the MME
  //Create response
  S1ApInitialUeMessage *initialUeMessage = new S1ApInitialUeMessage;
  initialUeMessage->set_enb_ue_s1ap_id(17 * message.uecrnti());
  initialUeMessage->set_epsattachtype(EpsAttach);
  Imsi_message *tempImsi = new Imsi_message();
  *tempImsi = m_state->imsi;
  initialUeMessage->set_allocated_identity(tempImsi);
  //delete tempImsi?
  //Pack it into a S1Message
  S1Message s1Message;
  s1Message.set_messagetype(S1Message_MessageType_TypeS1ApIUeM);
  s1Message.set_allocated_messages1apiuem(initialUeMessage);
  //Serialize message
  string output_message;
  s1Message.SerializeToString(&output_message);

  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_mmeSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "S1Ap Initial Ue Message sent to Mme " << endl;

  //Wait for the response : voir comment ca va se passer avec plusieurs UEs
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(m_mmeSocket, incoming_data_buffer,1000, 0);
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
    
    S1Message s1Message;
    //const string str_message;
    string str_message(incoming_data_buffer, bytes_recieved);
    s1Message.ParseFromString(str_message);
    std::cout << "Message deserialized " << endl;
    S1ApInitialContextSetupRequest initialCSRequest;
    initialCSRequest = s1Message.messages1apicsrequest();

    //Modify the state
    m_state->securityKey = initialCSRequest.securitykey();
    m_state->epsBearerId = initialCSRequest.epsbearerid();

    cout << "InitialContextSetupRequest received " << endl;
    cout << "Value of id is : " << initialCSRequest.enb_ue_s1ap_id() << endl;

    //Need to send the SecurityModeCommand
    //Encrypt message
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
    
    

 //Create response
  SecurityModeCommand *securityMCommand = new SecurityModeCommand;
  securityMCommand->set_uecrnti(initialCSRequest.enb_ue_s1ap_id()/17);
  securityMCommand->set_message_security(encryptedMessage);
  //Pack it into a RrcMessage
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeSecurityMCommand);
  rrcMessage.set_allocated_messagesecuritymcommand(securityMCommand);
  //Serialize message
  string output_message;
  rrcMessage.SerializeToString(&output_message);
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "Security Mode Command sent " << endl;
  }
}

  /*void UeContextEnb::handleS1ApInitialContextSetupRequest(S1ApInitialContextSetupRequest message){
  //Print message
  cout << "Message S1Ap Initial Context Setup Request received " << endl;
  cout << "Value of id is : " << message.enb_ue_s1ap_id() << endl;
  }*/

void UeContextEnb::handleSecurityModeComplete(SecurityModeComplete message){
  //Print message
  cout << "Message SecurityModeComplete received " << endl;
  RrcMessage rrcMessage;
  if(message.securitymodesuccess()){
    //Success
    UeCapabilityEnquiry *ueCE = new UeCapabilityEnquiry;
    ueCE->set_uecrnti(message.uecrnti());
    ueCE->add_uecapabilityrequest(E_UTRA);
    ueCE->add_uecapabilityrequest(UTRA);
    ueCE->add_uecapabilityrequest(GERAN_CS);
    ueCE->add_uecapabilityrequest(GERAN_PS);
    ueCE->add_uecapabilityrequest(CDMA2000);
    //Pack it
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeUeCE);
    rrcMessage.set_allocated_messageuece(ueCE);
  }
  else {
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.uecrnti());
    rrcCReject->set_waitingtime((message.uecrnti() % 15)+1);
    //Pack it
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);
    //Change state back to idle
    m_state->rrcState = RRC_Idle;
  }
  //Serialize message
  string output_message;
  rrcMessage.SerializeToString(&output_message);
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "UeCapabilityEnquiry or Reject sent " << endl;
  cout << bytes_sent << " bytes sent " << endl;
  cout << "M_state crnti : "<<  m_state->c_rnti << endl;
}

void UeContextEnb::handleUeCapabilityInformation(UeCapabilityInformation message){
  //Print message
  cout << "Message UeCI received " << endl;
  //Create response
  RrcConnectionReconfiguration *rrcCReconfiguration = new RrcConnectionReconfiguration;
  rrcCReconfiguration->set_uecrnti(message.uecrnti());
  rrcCReconfiguration->set_epsradiobeareridentity(m_state->epsBearerId);
  //Pack it into a RrcMessage
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReconfiguration);
  rrcMessage.set_allocated_messagerrccreconfiguration(rrcCReconfiguration);
  //Serialize message
  string output_message;
  rrcMessage.SerializeToString(&output_message);
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "RrcConnectionReconfiguration sent " << endl;
  cout << "M_state crnti : "<<  m_state->c_rnti << endl;
}

void UeContextEnb::handleRrcConnectionReconfigurationComplete (RrcConnectionReconfigurationComplete message){
  //Print message
  cout << "Message RrcCRC received " << endl;
  
  if(message.epsradiobeareractivated()){
    //Success
    S1Message s1Message;
    S1ApInitialContextSetupResponse *iCSResponse = new S1ApInitialContextSetupResponse;
    iCSResponse->set_enb_ue_s1ap_id(m_state->enbUeS1ApId);
    cout << "M_state enb2 : "<<  m_state->enbUeS1ApId << endl;
    cout << "M_state crnti : "<<  m_state->c_rnti << endl;
    string *erabId = new string;
    genRandId(erabId,5);
    iCSResponse->set_allocated_erabid(erabId);
    //Pack it
    s1Message.set_messagetype(S1Message_MessageType_TypeS1ApICSResponse);
    s1Message.set_allocated_messages1apicsresponse(iCSResponse);
    string output_message;
  s1Message.SerializeToString(&output_message);

  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_mmeSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "To MME sent " << endl;
  cout << bytes_sent << " bytes sent " << endl;
  RrcMessage rrcMessage;
    RrcConnectionAccept *rrcCA = new RrcConnectionAccept;
    rrcCA->set_uecrnti(message.uecrnti());
     //Pack it
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCA);
    rrcMessage.set_allocated_messagerrcca(rrcCA);
    rrcMessage.SerializeToString(&output_message);
  
  //Send Response
  //int len;
  //ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "Accept sent " << endl;
  cout << bytes_sent << " bytes sent " << endl;
  printState();
  }
  else {
    RrcMessage rrcMessage;
    RrcConnectionReject *rrcCReject = new RrcConnectionReject;
    rrcCReject->set_uecrnti(message.uecrnti());
    rrcCReject->set_waitingtime((message.uecrnti() % 15)+1);
    //Pack it
    rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRrcCReject);
    rrcMessage.set_allocated_messagerrccreject(rrcCReject);
    //Change state back to idle
    m_state->rrcState = RRC_Idle;
    string output_message;
    rrcMessage.SerializeToString(&output_message);

  //Serialize message
  
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "Reject sent " << endl;
  cout << bytes_sent << " bytes sent " << endl;
  }
}

void UeContextEnb::printState(){
  ostringstream state;
  state << "Context at the end: UeContextEnb {RRC state: " << m_state->rrcState << "}" << endl; 
  m_log->writeToLog(state.str());
}
