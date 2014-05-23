#include "UeContextMme.hh"
#include <iostream>

using namespace std;

UeContextMme::UeContextMme(int enbSocket):m_enbSocket(enbSocket)
{
  //TODO: initialize the m_state to default values
} 

void UeContextMme::handleS1ApInitialUeMessage(S1ApInitialUeMessage message){
//Print message
  cout << "Message Initial UE message received " << endl;
  cout << "Id is : " << message.enb_ue_s1ap_id() << endl;

}
 
void UeContextMme::handleS1ApInitialContextSetupResponse(S1ApInitialContextSetupResponse message){
}
