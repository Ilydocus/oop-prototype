#include <iostream>
#include "ue.h"
#include "Identifiers.h"
#include <cstring> //for memset
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>


using namespace std;

void CreateRaPreamble (RaPreamble *rapreamble, int ueId) {

  rapreamble->set_ueidrntitype(RA_RNTI);
  rapreamble->set_ueidrntivalue(ueId);

}

void sendRaPreamble (int socketfd, int ueId){
  RaPreamble *rapreamble = new RaPreamble;
  CreateRaPreamble (rapreamble, ueId);
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

  bytes_sent = send (socketfd, message.c_str(), 
		     message.length(), 0);

  std::cout << "Message RAPreamble sent" << std::endl;
  std::cout << "Bytes sent: "<<bytes_sent << std::endl; 
}

void handleRaResponse (int socketfd, UeContext_ue *ue_state) {
  //Part 1: Recieve the message
ssize_t bytes_recieved;
		     char incoming_data_buffer[1000];
		     bytes_recieved = recv(socketfd, incoming_data_buffer,1000, 0);
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
  Imsi_message *tempImsi = new Imsi_message(ue_state->imsi);
  //tempImsi = ue_state->imsi;
  rrcConnectionRequest->set_allocated_ueidentity(tempImsi);
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
  bytes_sent = send (socketfd, message.c_str(), 
		     message.length(), 0);

  std::cout << "Message RrcConnectionRequest sent" << std::endl;
  std::cout << "Bytes sent: "<<bytes_sent << std::endl;
		     }
		     
}

bool handleRrcConnectionSetup (int socketfd, UeContext_ue *ue_state) {
  //Part 1: Recieve the message
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(socketfd, incoming_data_buffer,1000, 0);
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
      ue_state->srbId = rrcConnectionSetup.srbidentity();
      //Send Response
      RrcConnectionSetupComplete *rrcCSC = new RrcConnectionSetupComplete;
      rrcCSC->set_uecrnti(rrcConnectionSetup.ueidrntivalue());
      rrcCSC->set_selectedplmnidentity((ue_state->imsi).mcc() + (ue_state->imsi).mnc());
      //Pack it into a RrcMessage
      RrcMessage rrcMessage_o;
      rrcMessage_o.set_messagetype(RrcMessage_MessageType_TypeRrcCSC);
      rrcMessage_o.set_allocated_messagerrccsc(rrcCSC);
      //Serialize the message
      std::string message;
      rrcMessage_o.SerializeToString(&message);
      std::cout << "Serialization completed " << std::endl;

      ssize_t bytes_sent;
      bytes_sent = send (socketfd, message.c_str(), 
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

void *powerOn (void * ueId_void){

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int ueId = *((int *) ueId_void);
  //free (ueId_void);

  //Create a socket and connect it
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;

  memset(&host_info, 0, sizeof host_info);
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo("127.0.0.1","43000",&host_info, &host_info_list);
  //status != 0 -error
  int socketfd;
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  //use first one in the list
  //socket == 1 -error

  status = connect (socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);

  //Initialize the UE state
  UeContext_ue * ue_state= new UeContext_ue;
  Imsi_message * temp_imsi = new Imsi_message;
  genImsi(temp_imsi);
  ue_state->imsi = *temp_imsi;
  ue_state->securityKey = ueId * 18;

  //Connection setup procedure
  sendRaPreamble (socketfd, ueId);
  handleRaResponse (socketfd, ue_state);
  bool reject = handleRrcConnectionSetup (socketfd, ue_state);
 

  

  //Termination of the procedure
  google::protobuf::ShutdownProtobufLibrary();
  std::cout << "Stopping server..." << std::endl;
  freeaddrinfo(host_info_list);
  close(socketfd);
  
  pthread_exit(NULL);
}


int main () {

  const int nbOfUes = 1;
  pthread_t thread[nbOfUes];
  int temp_arg[nbOfUes];
  //Aim:powerOn several UEs in different threads
  for (int i = 1; i<nbOfUes+1;i++){
    
    temp_arg[i-1]= i;
    pthread_create (&thread[i -1], NULL, powerOn,static_cast<void*>(&temp_arg[i-1]));
    
  }
  //Wait
  for (int i = 1; i<=nbOfUes;i++){

    pthread_join(thread[i-1],NULL);
  }
  //sleep(5); //should not be needed

  return 0;
}

void genImsi(Imsi_message *imsi){
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



