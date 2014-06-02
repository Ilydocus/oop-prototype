#ifndef DEF_MMEEVENTHANDLER
#define DEF_MMEEVENTHANDLER

#include "UeContextMme.hh"
#include "EventHandler.hh"

class MmeEventHandler: public EventHandler{
public:
  MmeEventHandler();
  void run();

private:
  std::map<int,UeContextMme> mUeContexts;
  int mListenSocket;
  int mNbMessages;

  void handleNewUe(int connSock);
  void handleEnbMessage(S1Message s1Message,UeContextMme ueContext);

};

#endif
