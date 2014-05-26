#ifndef DEF_UECONTEXT
#define DEF_UECONTEXT

#include <string>
#include "RrcMessages.pb.h"
#include "S1Messages.pb.h"
#include "Log.hh"

using namespace std;

class UeContext {
protected:
  void genRandId(string * id,const int len);
  void sendRrcMessage(int socket, RrcMessage message);
  void sendS1Message(int socket, S1Message message);
  int receiveRrcMessage(int socket,RrcMessage *message);
  int receiveS1Message(int socket,S1Message *message);
  Log *m_log;
};

#endif
