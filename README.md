oop-prototype
=============

#Description

Implementation of a simplified version of the "attach UE" procedure in the RRC protocol of LTE, using the object-oriented programming paradigm and C++.

The protoype is composed of three programs that represent the behaviour of the three types of actors in the procedure:

- the UEs (in ue.cc)
- the eNodeB (in eNodeB.cc)
- the MME (in mme.cc)

Different information about the UEs are stored in the three actors.

The three programs communicate using TCP sockets and the incoming of new data on the sockets is handled using epoll.

Serialization of the messages is done using [Google Protocol Buffers](https://developers.google.com/protocol-buffers/).

To simulate the behavior of the SecurityMode messages, part of the SecurityModeCommand message is encrypted using [Crypto++](http://cryptopp.com/).


