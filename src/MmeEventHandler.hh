#ifndef DEF_MMEEVENTHANDLER
#define DEF_MMEEVENTHANDLER

#include "UeContextMme.hh"
#include "EventHandler.hh"

class MmeEventHandler: public EventHandler{
  public:
  MmeEventHandler();
  void run();

  private:
  map<int,UeContextMme> mUeContexts;
  int mListenSocket;
  int mNbMessages;

  void handleNewUe(int connSock);
  void handleUeMessage(S1Message rrcMessage,UeContextMme ueContext);

};

#endif
