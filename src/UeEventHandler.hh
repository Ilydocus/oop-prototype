#ifndef DEF_UEEVENTHANDLER
#define DEF_UEEVENTHANDLER

#include "UeContextUe.hh"
#include "EventHandler.hh"

class UeEventHandler: public EventHandler{
public:
  UeEventHandler();
  ~UeEventHandler();
  void run();
  void powerOnUes(int nbOfUes);

private:
  typedef std::map<int,UeContextUe*> UeMap;
  int mEpollfd;
  int mNbCompletedUes;
  int mNbCreatedUes;
  UeMap mUeContexts;

  void handleEnbMessage(RrcMessage rrcMessage,UeContextUe *ueContext);
  void handleNewUe(int connSock,int ueId);
};

#endif
