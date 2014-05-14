#include "UeContext.h"
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

UeContext::UeContext(int ueSocket, int mmeSocket):m_ueSocket(ueSocket),m_mmeSocket(mmeSocket)
{
}

void UeContext::handleRaPreamble(RaPreamble message)
{
  //Print message
  cout << "Message RA Preamble received " << endl;
  cout << "Type of rnti is : " << message.ueidrntitype() << endl;
  cout << "Value of rnti is : " << message.ueidrntivalue() << endl;
  //Create response
  RaResponse *raResponse = new RaResponse;
  raResponse->set_ueidrntitype(RA_RNTI);
  raResponse->set_ueidrntivalue(message.ueidrntivalue());
  raResponse->set_ueidcrnti(message.ueidrntivalue());
  //Pack it into a RrcMessage
  RrcMessage rrcMessage;
  rrcMessage.set_messagetype(RrcMessage_MessageType_TypeRaResponse);
  rrcMessage.set_allocated_messagerar(raResponse);
  //Serialize message
  string output_message;
  rrcMessage.SerializeToString(&output_message);
  //Modify state
  m_state = message.ueidrntivalue();
  cout << "State is : " << m_state << endl;
  //Send Response
  int len;
  ssize_t bytes_sent;

  bytes_sent = send (m_ueSocket, output_message.c_str(), 
		     output_message.length(), 0);
  cout << "RA Response sent " << endl;
  
}
