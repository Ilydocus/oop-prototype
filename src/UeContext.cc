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
  string outputMessage;
  message.SerializeToString(&outputMessage);

  int len;
  ssize_t bytesSent;
  bytesSent = send (socket, outputMessage.c_str(), 
		     outputMessage.length(), 0);
}

void UeContext::sendS1Message(int socket, S1Message message){
  string outputMessage;
  message.SerializeToString(&outputMessage);

  int len;
  ssize_t bytesSent;
  bytesSent = send (socket, outputMessage.c_str(), 
		     outputMessage.length(), 0);
}
 
int UeContext::receiveRrcMessage(int socket,RrcMessage *message){
  ssize_t bytesRecieved;
  char incomingDataBuffer[1000];
  bytesRecieved = recv(socket, incomingDataBuffer,1000, 0);
  if (bytesRecieved == 0) {std::cout << "host shut down." << std::endl;return 0;}
  if (bytesRecieved == -1){std::cout << "recieve error!" << std::endl;return 0;}
  if (bytesRecieved != -1 && bytesRecieved != 0){
    incomingDataBuffer[bytesRecieved] = '\0';

    GOOGLE_PROTOBUF_VERIFY_VERSION;  
    string strMessage(incomingDataBuffer, bytesRecieved);
    message->ParseFromString(strMessage);
    return 1;
  }
}

int UeContext::receiveS1Message(int socket,S1Message *message){
  ssize_t bytesRecieved;
  char incomingDataBuffer[1000];
  bytesRecieved = recv(socket, incomingDataBuffer,1000, 0);
  if (bytesRecieved == 0) {std::cout << "host shut down." << std::endl;return 0;}
  if (bytesRecieved == -1){std::cout << "recieve error!" << std::endl;return 0;}
  if (bytesRecieved != -1 && bytesRecieved != 0){
    incomingDataBuffer[bytesRecieved] = '\0';

    GOOGLE_PROTOBUF_VERIFY_VERSION;  
    string strMessage(incomingDataBuffer, bytesRecieved);
    message->ParseFromString(strMessage);
    return 1;
  }
}

