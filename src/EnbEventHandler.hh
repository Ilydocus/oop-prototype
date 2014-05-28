#ifndef DEF_ENBEVENTHANDLER
#define DEF_ENBEVENTHANDLER

#include "UeContextEnb.hh"
#include "EventHandler.hh"

class EnbEventHandler: public EventHandler{
public:
  EnbEventHandler();
  void run();

private:
  std::map<int,UeContextEnb> mUeContexts;
  int mListenSocket;
  int mMmeSocket;
  int mNbMessages;

  void handleNewUe(int connSock);
  void handleUeMessage(RrcMessage rrcMessage,UeContextEnb ueContext);
};

#endif
