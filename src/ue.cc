#include "UeEventHandler.hh"
#include <pthread.h>

using namespace std;

struct arg_struct{
  int ueId;
  Log *log;
};

void *powerOn (void * arguments){
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  arg_struct *args = (arg_struct *)arguments;

  UeEventHandler *eventHandler = new UeEventHandler(args->ueId,args->log);
  eventHandler->run();
  delete eventHandler;
}



int main () {

  Log *log = new Log("UesLog.txt");

  const int nbOfUes = 100;
  pthread_t thread[nbOfUes];
  arg_struct temp_arg[nbOfUes];

  for (int i = 1; i<nbOfUes+1;i++){
    temp_arg[i-1].ueId= i;
    temp_arg[i-1].log= log;
    pthread_create (&thread[i -1], NULL, powerOn,static_cast<void*>(&temp_arg[i-1]));
    
  }
  for (int i = 1; i<=nbOfUes;i++){

    pthread_join(thread[i-1],NULL);
  }
  delete log;
  return 0;
}





