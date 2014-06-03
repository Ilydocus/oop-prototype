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
  typedef std::map<int,UeContextEnb*> UeMapUe;
  typedef std::map<int,UeContextEnb*> UeMapMme;
  UeMapUe mUeContextsUe;
  UeMapMme mUeContextsMme;
  int mListenSocket;
  int mMmeSocket;
  int mNbMessages;

  void handleNewUe(int connSock);
  void handleUeMessage(RrcMessage rrcMessage,UeContextEnb *ueContext);
  void handleMmeMessage(S1Message S1Message);
};

#endif
