#ifndef DEF_UEEVENTHANDLER
#define DEF_UEEVENTHANDLER

#include "UeContextUe.hh"
#include "EventHandler.hh"

class UeEventHandler: public EventHandler{
public:
  UeEventHandler(Log *log);
  ~UeEventHandler();
  void run();
  void powerOnUes(int nbOfUes);

private:
  int mEpollfd;
  int mNbCompletedUes;
  int mNbCreatedUes;
  std::map<int,UeContextUe> mUeContexts;

  void handleEnbMessage(RrcMessage rrcMessage,UeContextUe ueContext);
  void handleNewUe(int connSock,int ueId);
};

#endif
