#ifndef DEF_ENBEVENTHANDLER
#define DEF_ENBEVENTHANDLER

#include "UeContextEnb.hh"
#include "EventHandler.hh"

class EnbEventHandler: public EventHandler{
public:
  EnbEventHandler();
  ~EnbEventHandler();
  void run();

private:
  typedef std::map<int,UeContextEnb*> UeMap;
  UeMap mUeContexts;
  int mListenSocket;
  int mMmeSocket;
  int mNbMessages;

  void handleNewUe(int connSock);
  void handleUeMessage(RrcMessage rrcMessage,UeContextEnb *ueContext);
};

#endif
