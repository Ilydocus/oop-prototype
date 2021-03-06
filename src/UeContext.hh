#ifndef DEF_UECONTEXT
#define DEF_UECONTEXT

#include <string>
#include "RrcMessages.pb.h"
#include "S1Messages.pb.h"
#include "Log.hh"

class UeContext {
protected:
  void genRandId(std::string * id,const int len);
  void sendRrcMessage(int socket, RrcMessage message);
  void sendS1Message(int socket, S1Message message);
  virtual void printState() =0;
  Log *mLog;
};

#endif
