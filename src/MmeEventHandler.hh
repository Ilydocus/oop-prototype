#ifndef DEF_MMEEVENTHANDLER
#define DEF_MMEEVENTHANDLER

#include "UeContextMme.hh"
#include "EventHandler.hh"

class MmeEventHandler: public EventHandler{
public:
  MmeEventHandler();
  ~MmeEventHandler();
  void run();

private:
  typedef std::map<int,UeContextMme*> UeMap;
  UeMap mUeContexts;
  int mListenSocket;
  int mNbMessages;

  void handleNewUe(int connSock);
  void handleEnbMessage(S1Message s1Message,UeContextMme *ueContext);

};

#endif
