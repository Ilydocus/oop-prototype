oop-prototype
=============

#Description

Implementation of a simplified version of the connection setup procedure of the RRC protocol in LTE, using the object-oriented programming paradigm and C++.

The protoype is composed of three programs that represent the behaviour of the three types of actors in the procedure:

- the UEs (in ue.cc)
- the eNodeB (in eNodeB.cc)
- the MME (in mme.cc)

Different information about the UEs are stored in the three actors.

The three programs communicate using TCP sockets and the incoming of new data on the sockets is handled using epoll.

Serialization of the messages is done using [Google Protocol Buffers](https://developers.google.com/protocol-buffers/).

To simulate the behavior of the SecurityMode messages, part of the SecurityModeCommand message is encrypted using [Crypto++](http://cryptopp.com/).

#RRC connection setup procedure

Several sequence diagrams describing the messaging sequence according to different use cases can be found in a document in the doc folder.

#How does it work?

##For the UE program

The UEs are created in the method powerOnUes (UeEventHandler.cc, l.27).

The first message (RaPreamble) is sent from within the epoll loop, line 98 (UeEventHandler.cc).
Subsequent messages are received by the event loop and handled by the method handleEnbMessage (UeEventHandler.cc, l.124 and l.146).

The methods to handle the different messages are in the file UeContextUe.cc

##For the eNodeB program

The messages coming from the UEs and the MME are handled in the epoll loop. The code for messages coming from the UEs begins line 196 (EnbEventHandler.cc), for messages coming from the MME it begins line 128 (EnbEventHandler.cc).

The methods to handle the different messages are in the file UeContextEnb.cc

##For the MME program

The messages coming from the eNodeB are handled in the epoll loop from line 116 (MmeEventHandler.cc).

The methods to handle the different messages are in the file UeContextMme.cc

##For the messages

RRC and S1 messages are described in the RrcMessages.proto and S1Messages.proto files, from which the Protocol Buffers API generates the related C++ files.

