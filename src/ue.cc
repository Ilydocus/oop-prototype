#include "UeEventHandler.hh"
#include <pthread.h>



using namespace std;

void *powerOn (void * ueId_void){

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int ueId = *((int *) ueId_void);
  //free (ueId_void);

  UeEventHandler *eventHandler = new UeEventHandler(ueId);
  eventHandler->run();
  //Destroy the UE?
  delete eventHandler;
}


int main () {

  const int nbOfUes = 1;
  pthread_t thread[nbOfUes];
  int temp_arg[nbOfUes];

  //Aim:powerOn several UEs in different threads
  for (int i = 1; i<nbOfUes+1;i++){
    
    temp_arg[i-1]= i;
    pthread_create (&thread[i -1], NULL, powerOn,static_cast<void*>(&temp_arg[i-1]));
    
  }
  //Wait
  for (int i = 1; i<=nbOfUes;i++){

    pthread_join(thread[i-1],NULL);
  }

  return 0;
}





