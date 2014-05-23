#include "UeContextMme.hh"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

UeContextMme::UeContextMme(int enbSocket):m_enbSocket(enbSocket)
{
  //TODO: initialize the m_state to default values
} 

void UeContextMme::handleS1ApInitialUeMessage(S1ApInitialUeMessage message){
//Print message
  cout << "Message Initial UE message received " << endl;
  cout << "Id is : " << message.enb_ue_s1ap_id() << endl;
  //Create Eps bearer id
  string * epsBearerId = new string;
  genRandId(epsBearerId, 8);
  //Send response
  S1ApInitialContextSetupRequest *initialCSRequest = new S1ApInitialContextSetupRequest;
  initialCSRequest->set_mme_ue_s1ap_id(message.enb_ue_s1ap_id() - 4);
  initialCSRequest->set_enb_ue_s1ap_id(message.enb_ue_s1ap_id());
  initialCSRequest->set_securitykey((message.enb_ue_s1ap_id() / 17 )*18);
  initialCSRequest->set_epsbearerid(*epsBearerId);
   //delete epsBearerId?
  //Pack it into a S1Message
  S1Message s1Message;
  s1Message.set_messagetype(S1Message_MessageType_TypeS1ApICSRequest);
  s1Message.set_allocated_messages1apicsrequest(initialCSRequest);
  //Serialize message
  string output_message;
  s1Message.SerializeToString(&output_message);

  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_enbSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "S1Ap Initial Context Request sent to ENodeB " << endl;

}
 
void UeContextMme::handleS1ApInitialContextSetupResponse(S1ApInitialContextSetupResponse message){
}
