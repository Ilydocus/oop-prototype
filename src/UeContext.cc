#include "UeContext.hh"

#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

void UeContext::genRandId(string * id,const int len){
  char temp[len];
  static const char num[11] = "0123456789";
  for (int i = 0; i<len; ++i){
    temp[i] = num[(int)((double)rand() / ((double)RAND_MAX + 1)*(sizeof(num)-1))];
  }
  temp[len]=0;
  string s(temp);
  *id = s;
}

void UeContext::sendRrcMessage(int socket, RrcMessage message){
  string output_message;
  message.SerializeToString(&output_message);

  int len;
  ssize_t bytes_sent;
  bytes_sent = send (socket, output_message.c_str(), 
		     output_message.length(), 0);
}

void UeContext::sendS1Message(int socket, S1Message message){
  string output_message;
  message.SerializeToString(&output_message);

  int len;
  ssize_t bytes_sent;
  bytes_sent = send (socket, output_message.c_str(), 
		     output_message.length(), 0);
}
 
int UeContext::receiveRrcMessage(int socket,RrcMessage *message){
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(socket, incoming_data_buffer,1000, 0);
  if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl;return 0;}
  if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl;return 0;}
  if (bytes_recieved != -1 && bytes_recieved != 0){
    incoming_data_buffer[bytes_recieved] = '\0';

    GOOGLE_PROTOBUF_VERIFY_VERSION;  
    string str_message(incoming_data_buffer, bytes_recieved);
    message->ParseFromString(str_message);
    return 1;
  }
}

int UeContext::receiveS1Message(int socket,S1Message *message){
  ssize_t bytes_recieved;
  char incoming_data_buffer[1000];
  bytes_recieved = recv(socket, incoming_data_buffer,1000, 0);
  if (bytes_recieved == 0) {std::cout << "host shut down." << std::endl;return 0;}
  if (bytes_recieved == -1){std::cout << "recieve error!" << std::endl;return 0;}
  if (bytes_recieved != -1 && bytes_recieved != 0){
    incoming_data_buffer[bytes_recieved] = '\0';

    GOOGLE_PROTOBUF_VERIFY_VERSION;  
    string str_message(incoming_data_buffer, bytes_recieved);
    message->ParseFromString(str_message);
    return 1;
  }
}

