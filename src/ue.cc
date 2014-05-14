#include <string>
#include <iostream>
#include "RrcMessages.pb.h"
#include <cstring> //for memset
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

void CreateRaPreamble (RaPreamble *rapreamble) {

  rapreamble->set_ueidrntitype(RA_RNTI);
  rapreamble->set_ueidrntivalue(456);

}

int main () {

  GOOGLE_PROTOBUF_VERIFY_VERSION;
  //Create the RaPreamble
  RaPreamble *rapreamble = new RaPreamble;
  CreateRaPreamble (rapreamble);
  std::cout << "Ra_rnti is : " << rapreamble->ueidrntivalue() << std::endl;
  //Pack it into a RrcMessage
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaPreamble);
  rrcMessage.set_allocated_messagerap(rapreamble);
  //Serialize the message
  std::string message;
  rrcMessage.SerializeToString(&message);
  std::cout << "Serialization completed " << std::endl;
  //Conversion of the message
  const char * ser_message;
  ser_message = message.c_str();
  //Socket code
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
  cout << "Value of rnti is : " << raResponse.ueidrntivalue() << endl;}

  //Test: is deserialization working?
    /*RaPreamble test;
    test.ParseFromString(message);
    std::cout << "Ra_rnti is : " << test.ueidrntivalue() << std::endl;
    std::cout << "Type of rnti is : " << test.ueidrntitype() << std::endl;*/


  //For freeing memory
  google::protobuf::ShutdownProtobufLibrary();
  std::cout << "Stopping server..." << std::endl;
  freeaddrinfo(host_info_list);
  close(socketfd);

  return 0;
}


