///////////////////////////////////////////////////////////////////////////////
// MQ4CPP - Message queuing for C++
// Copyright (C) 2004-2007  Riccardo Pompeo (Italy)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <strstream>
using namespace std;

#define SILENT
#include "MessageProxy.h"
#include "Trace.h"
#include "Logger.h"
#include "GeneralHashFunctions.h"

#define SYNCVAL 0xbeef
#define MAX_CONNECTIONS 100

NetworkMessage::NetworkMessage(NetworkMessage& o) // ++ v1.5
	   		   :Message("NetworkMessage")
{
	itsTopic=o.itsTopic;
	itsBuffer=o.itsBuffer;
	itsTarget=o.itsTarget;	
	itsSender=o.itsSender;
	itsSeqNum=o.itsSeqNum;
	itsUnsolicitedFlag=o.itsUnsolicitedFlag;
	itsBroadcastFlag=o.itsBroadcastFlag;
} 

NetworkMessage::NetworkMessage(char* theBuffer, unsigned short theLen) 
	   		   :Message("NetworkMessage"), 
	    	    itsTarget(0), itsRemoteSender(0), itsSeqNum(0), 
	    	    itsUnsolicitedFlag(false), itsBroadcastFlag(false)
{
	if(theLen > 0xFFFF - sizeof(NetworkMessage::NetworkMessageHeader))
		throw ThreadException("NetworkMessage is exceding permitted size");
	itsBuffer.assign(theBuffer,theLen);
}

NetworkMessage::NetworkMessage(string theBuffer) 
	   		   :Message("NetworkMessage"), 
	    	    itsTarget(0), itsRemoteSender(0),itsSeqNum(0), 
	    	    itsUnsolicitedFlag(false), itsBroadcastFlag(false)
{
	if(theBuffer.length() > 0xFFFF - sizeof(NetworkMessage::NetworkMessageHeader))
		throw ThreadException("NetworkMessage is exceding permitted size");
	itsBuffer=theBuffer;
}

string NetworkMessage::toString()
{
	NetworkMessage::NetworkMessageHeader anHeader;
	if(itsBuffer.length() > 0xFFFF - sizeof(NetworkMessage::NetworkMessageHeader))
		throw ThreadException("NetworkMessage is exceding permitted size");
			
	anHeader.sender=itsSender;
	anHeader.seqnum=itsSeqNum; // ++ v1.1
	anHeader.buflen=itsBuffer.length();
	anHeader.topiclen=itsTopic.length(); // ++ v1.5
	string aBuffer;
	aBuffer.assign((char*)&anHeader,sizeof(anHeader));
	aBuffer+=itsTopic; // ++ v1.5
	aBuffer+=itsBuffer;
	return aBuffer;	
}

void NetworkMessage::toStream(ostream& theStream)
{
	theStream.write(itsBuffer.c_str(),itsBuffer.length());
}

void NetworkMessage::code(Encription* theEncr) 
{	
	itsBuffer=theEncr->code(itsBuffer);
}

void NetworkMessage::decode(Encription* theEncr)
{
	itsBuffer=theEncr->decode(itsBuffer);
}

void NetworkMessage::inflate(Compression* theCompr) 
{	
	itsBuffer=theCompr->inflate(itsBuffer);
}

void NetworkMessage::deflate(Compression* theCompr)
{
	itsBuffer=theCompr->deflate(itsBuffer);
}

PingRequestMessage::PingRequestMessage(MQHANDLE theSenderID) 
	   		  	   :Message("PingRequestMessage")
{
	itsSender=theSenderID;
}	   		  

string PingRequestMessage::toString()
{
	PingRequest anHeader;
	anHeader.sender=itsSender;
	string aBuffer;
	aBuffer.assign((char*)&anHeader,sizeof(anHeader));
	return aBuffer;	
}

PingReplyMessage::PingReplyMessage(MQHANDLE theTarget)
	   		  	 :Message("PingReplyMessage")
{
	itsTarget=theTarget;
}

LookupRequestMessage::LookupRequestMessage(const char* theName,MQHANDLE theSenderID) 
	   		  	     :Message("LookupRequestMessage")
{
	itsNameToLookup=theName;
	itsSender=theSenderID;
}	   		  

string LookupRequestMessage::toString()
{
	LookupRequest anHeader;
	unsigned aLen=itsNameToLookup.length();
	if(aLen > 0xFFFF - sizeof(anHeader))
		throw ThreadException("LookupRequestMessage is exceding permitted size");

	anHeader.sender=itsSender;
	anHeader.namelen=aLen;
	string aBuffer;
	aBuffer.assign((char*)&anHeader,sizeof(anHeader));
	aBuffer+=itsNameToLookup;
	return aBuffer;	
}

LookupReplyMessage::LookupReplyMessage()
	   		  	   :Message("LookupReplyMessage")
{
	itsBuffer.fail=true;
	itsBuffer.handle=0;
	itsTarget=0;
}

LookupReplyMessage::LookupReplyMessage(MQHANDLE theTarget)
	   		  	   :Message("LookupReplyMessage")
{
	itsBuffer.fail=true;
	itsBuffer.handle=0;
	itsTarget=theTarget;
}

LookupReplyMessage::LookupReplyMessage(LookupReplyMessage::LookupReply& theReply)
	   		  	   :Message("LookupReplyMessage")
{
	itsBuffer.fail=theReply.fail;
	itsBuffer.handle=theReply.handle;
	itsTarget=0;
}

LookupReplyMessage::LookupReplyMessage(MQHANDLE theTarget,MQHANDLE theHandle)
	   		  	   :Message("LookupReplyMessage")
{
	itsBuffer.fail=false;
	itsBuffer.handle=theHandle;
	itsTarget=theTarget;
}
	
string LookupReplyMessage::toString()
{
	string aRet;
	aRet.assign((char*)&itsBuffer,sizeof(itsBuffer));
	return aRet;	
}

Observer::Observer(const char* theName)
		 :MessageQueue(theName) 
{
	TRACE("Observer::Observer - start")
	itsEncription=NULL;
	itsCompression=NULL;
	itsLastMessageProxy=0;
	TRACE("Observer::Observer - end")
}

Observer::~Observer()
{
	TRACE("Observer::~Observer - start")
	if(itsEncription!=NULL) 
		delete itsEncription; 
	TRACE("Observer::~Observer - end")
}

void Observer::setEncription(Encription* theEncr)
{ 
	TRACE("Observer::setEncription - start")
	if(itsEncription!=NULL) 
		delete itsEncription; 	
	itsEncription=theEncr; 
	TRACE("Observer::setEncription - end")
}

void Observer::setCompression(Compression* theCompr)
{ 
	TRACE("Observer::setCompression - start")
	if(itsCompression!=NULL) 
		delete itsCompression; 	
	itsCompression=theCompr; 
	TRACE("Observer::setCompression - end")
}

void Observer::post(MQHANDLE theTarget,NetworkMessage* theMessage)
{
	TRACE("Observer::post(static) - start")
	if(itsCompression!=NULL)
		theMessage->deflate(itsCompression);

	if(itsEncription!=NULL)
		theMessage->code(itsEncription);	

	MessageQueue::post(theTarget,theMessage);
	TRACE("Observer::post(static) - end")
}

void Observer::publish(string theTopic,string theMessage)
{
	TRACE("Observer::publish - start")
	NetworkMessage* aMessage=new NetworkMessage(theMessage);
	aMessage->setBroadcasting();
	aMessage->setTopic(theTopic);
	aMessage->setSender(getID());
	if(itsCompression!=NULL)
		aMessage->deflate(itsCompression);

	if(itsEncription!=NULL)
		aMessage->code(itsEncription);	

	MessageQueue::broadcast(aMessage);
	TRACE("Observer::publish - end")
}

void Observer::onMessage(Message* theMessage)
{
	TRACE("Observer::onMessage - start")
	TRACE("Thread name=" << getName())

	try
	{
		if(theMessage->is("Wakeup"))
		{
			TRACE("Call onWakeup")
			onWakeup((Wakeup*)theMessage);
		}
		else if(theMessage->is("PingReplyMessage"))
		{
			TRACE("Call onPing")
			onPing((PingReplyMessage*)theMessage);
		}
		else if(theMessage->is("LookupReplyMessage"))
		{
			TRACE("Call onLookup")
			onLookup((LookupReplyMessage*)theMessage);
		}
		else if(theMessage->is("NetworkMessage"))
		{
			NetworkMessage* aRequest=(NetworkMessage*)theMessage;
			itsLastMessageProxy=aRequest->getSender();	
			itsLastReceivedTopic=aRequest->getTopic();

			if(aRequest->isUnsolicited())
			{
				TRACE("Call onUnsolicited")
				if(itsEncription!=NULL)	aRequest->decode(itsEncription);
				if(itsCompression!=NULL) aRequest->inflate(itsCompression);
				onUnsolicited(aRequest);	
			}		
			else if(aRequest->isBroadcasting())
			{
				TRACE("Call onBroadcast")
				bool fire=false;
				if(itsTopicList.size()>0)
				{
					TRACE("Find enabled topics")
					for(vector<string>::iterator i = itsTopicList.begin(); i < itsTopicList.end(); ++i)
					{
						if(*i==aRequest->getTopic())
						{
							TRACE("Found topic=" << (*i).c_str())
							fire=true;
						}
					}	
				}
				
				if(fire==true)
				{
					if(itsEncription!=NULL)	aRequest->decode(itsEncription);
					if(itsCompression!=NULL) aRequest->inflate(itsCompression);
					onBroadcast(aRequest);	
				}
			}		
			else
			{
				TRACE("Call onNetworkMessage")
				if(itsEncription!=NULL)	aRequest->decode(itsEncription);
				if(itsCompression!=NULL) aRequest->inflate(itsCompression);
				NetworkMessage* aReply=onRequest(aRequest);
				if(aReply!=NULL)
				{
					aReply->setSender(getID());
					aReply->setTarget(aRequest->getRemoteSender());
					aReply->setSequenceNumber(aRequest->getSequenceNumber());
					post(aRequest->getSender(),aReply); 
				}
			}
		}
		else
		{
			TRACE("Call onLocal")
			onLocal(theMessage);
		}
	}
	catch(Exception& ex)
	{
		DISPLAY("Observer::onMessage(" << getName() << ") : " << ex.getMessage().c_str())
	}
	catch(...)
	{		
		DISPLAY("Observer::onMessage(" << getName() << ") : Unhandled exception")
	}

	TRACE("Observer::onMessage - end")
}

void Observer::decodeProperties(string& theBuffer,ListProperty& theProperties)
{
	TRACE("Observer::decodeProperties - start")
	theProperties.free();
	istrstream aStream(theBuffer.data(),theBuffer.length());
	theProperties.deserialize(aStream,true);
	TRACE("Observer::decodeProperties - end")
}

void Observer::decodeProperties(char* theBuffer,unsigned long theLen,ListProperty& theProperties)
{
	TRACE("Observer::decodeProperties - start")
	theProperties.free();
	istrstream aStream(theBuffer,theLen);
	theProperties.deserialize(aStream,true);
	TRACE("Observer::decodeProperties - end")
}

void Observer::encodeProperties(ListProperty& theProperties,string& theBuffer)
{
	TRACE("Observer::encodeProperties - start")
	ostrstream aStream;
	theProperties.serialize(aStream);
	int aLen=aStream.pcount();
	char* aBuffer=aStream.str();
	theBuffer.assign(aBuffer,aLen);
	delete [] aBuffer;
	TRACE("Observer::encodeProperties - end")
}

MessageProxy::MessageProxy(const char* theName)
			 :MessageQueue(theName)
{
	TRACE("MessageProxy constructor - start")
	TRACE("Name=" << theName)
	itsSocket=NULL;
	TRACE("MessageProxy constructor - end")
}

MessageProxy::MessageProxy(const char* theName,Socket* theSocket)
			 :MessageQueue(theName), itsSocket(theSocket)
{
	TRACE("MessageProxy constructor - start")
	TRACE("Name=" << theName)
	
#ifdef WIN32	
	DWORD tid = 0;	
	m_hThreadRx = (unsigned long*)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)_mp_thread_proc,(Thread*)this,0,&tid);
	if(m_hThreadRx == NULL) 
		throw ThreadException("Failed to create thread");
#else
    int iret = pthread_create( &m_hThreadRx, NULL, _mp_thread_proc,this);
    if(iret!=0)
     	throw ThreadException("Failed to create thread");
#endif
	TRACE("MessageProxy constructor - end")
}

MessageProxy::~MessageProxy()
{
	TRACE("MessageProxy destructor - start")	

	stop(false);
	if(itsSocket!=NULL)
	{
		itsSocket->Close();

#ifdef WIN32
		WaitForSingleObject(m_hThreadRx,INFINITE);
		CloseHandle(m_hThreadRx);
#else
		pthread_join(m_hThreadRx,NULL);
#endif
	}
	
	TRACE("MessageProxy destructor - end")	
}

string MessageProxy::getConnectionAddress(MQHANDLE theCaller,int& thePort)
{
	TRACE("MessageProxy::getConnectionAddress - start")
	char anAddr[20];
	string aName=getName();
	istrstream aStream(aName.c_str(),aName.size());
	aStream.ignore(sizeof(MESSAGEPROXYHEADER)-1);
	aStream.getline(anAddr,sizeof(anAddr),',');
	aStream >> thePort;
	TRACE("MessageProxy::getConnectionAddress - end")
	return anAddr;
}

void MessageProxy::onMessage(Message* theMessage)
{	
	TRACE("MessageProxy::onMessage - start")
	TRACE("Thread name=" << getName())
	header anHeader;
	anHeader.sync=SYNCVAL;
		
	TRACE("Message=" << theMessage->getClass())

	try
	{
		if(theMessage->is("NetworkMessage"))
		{
			NetworkMessage* aMessage=(NetworkMessage*)theMessage;
			if(aMessage->isUnsolicited())		
				anHeader.type=MQ_PROXY_UNSOLICITED;
			else if(aMessage->isBroadcasting()) // ++ v1.5
				anHeader.type=MQ_PROXY_BROADCAST;
			else
				anHeader.type=MQ_PROXY_MESSAGE;
	
			anHeader.target=aMessage->getTarget();
		}
		else if(theMessage->is("LookupRequestMessage"))
		{
			anHeader.type=MQ_PROXY_LOOKUP_REQUEST;
			anHeader.target=0;
		}
		else if(theMessage->is("LookupReplyMessage"))
		{
			anHeader.type=MQ_PROXY_LOOKUP_REPLY;
			anHeader.target=((LookupReplyMessage*)theMessage)->getTarget();
		}
		else if(theMessage->is("PingRequestMessage"))
		{
			anHeader.type=MQ_PROXY_PING_REQUEST;
			anHeader.target=0;
		}
		else if(theMessage->is("PingReplyMessage"))
		{
			anHeader.type=MQ_PROXY_PING_REPLY;
			anHeader.target=((PingReplyMessage*)theMessage)->getTarget();
		}
		else
		{
			WARNING("Message not allowed. Skipped!")
			return;
		}	
	
		string aBuffer=theMessage->toString();
		int aLen=aBuffer.length();
		if(aLen + sizeof(NetworkMessage::NetworkMessageHeader) > 0xFFFF) // ++ v1.5
		{
			WARNING("Message too long. Dropped!")
			return;	
		}
	
		anHeader.msglen=aLen;
		TRACE("Type=" << anHeader.type)
		TRACE("Target=" << anHeader.target)
		TRACE("MsgLen=" << anHeader.msglen)
	
		if(anHeader.msglen>0)
		{
			DUMP("Tx header",(char*)&anHeader,sizeof(header));
			//itsSocket->SendBuffer(&anHeader,sizeof(header));
			aBuffer=string((char*)&anHeader,sizeof(header))+aBuffer;
			DUMP("Tx buffer",(char*)aBuffer.data(),aBuffer.length());
			itsSocket->SendBytes(aBuffer);
			//BUFFER((char*)&anHeader,sizeof(header))
		}
		else
		{
			WARNING("Posted an empty network message. Skipped!")	
		}
	}
	catch(Exception& ex)
	{
		WARNING(ex.getMessage().c_str())
	}
	catch(...)
	{		
		CRITICAL("Unhandled exception")
	}
	
	TRACE("MessageProxy::onMessage - end")
}

void MessageProxy::receive()
{	
	TRACE("MessageProxy::receive - start")
	TRACE("Thread name=" << getName())
	
	char* aBuffer=new char[0x10000];
	
	while (true) 
	{
		try
		{
			TESTCANCEL		
			TRACE("Wait a message")

			header anHeader;
			if(itsSocket->ReceiveBuffer(&anHeader,sizeof(header))==false)
			{
				WARNING("Socket Rx returns an error")
				break;
			}
	
			TESTCANCEL		
			DUMP("Rx header",(char*)&anHeader,sizeof(header));
	
			if(anHeader.sync==SYNCVAL)
			{
				TRACE("Valid sync")
				TRACE("Message lenght=" << anHeader.msglen)
	
				if(anHeader.msglen>0)
					if(itsSocket->ReceiveBuffer(aBuffer,anHeader.msglen)==false)
					{
						WARNING("Socket Rx returns an error")
						break;
					}

				//BUFFER((char*)&anHeader,sizeof(header))
	
				TESTCANCEL		
				DUMP("Rx message",aBuffer,anHeader.msglen);
	
				if(anHeader.type==MQ_PROXY_MESSAGE || anHeader.type==MQ_PROXY_UNSOLICITED || anHeader.type==MQ_PROXY_BROADCAST) // ++v1.5
				{
					TRACE("type==MQ_PROXY_MESSAGE OR MQ_PROXY_UNSOLICITED OR MQ_PROXY_BROADCAST")
					NetworkMessage::NetworkMessageHeader* aNMHeader=(NetworkMessage::NetworkMessageHeader*)aBuffer;
					if((aNMHeader->topiclen > 0xFFFF - sizeof(aNMHeader) - aNMHeader->buflen) || // ++ v1.5
					   (aNMHeader->buflen > 0xFFFF - sizeof(aNMHeader) - aNMHeader->topiclen)) 
					{
						WARNING("Buffer overflow detected. Drop connection!")
						break;	
					}
					
					DUMP("NetworkMessage Rx header",(char*)aNMHeader,sizeof(NetworkMessage::NetworkMessageHeader));
					TRACE("Sender=" << aNMHeader->sender)
					TRACE("Sequence=" << aNMHeader->seqnum)
					TRACE("Topic lenght=" << aNMHeader->topiclen) // ++ v1.5
					TRACE("Buffer lenght=" << aNMHeader->buflen)

					//BUFFER((char*)aNMHeader,sizeof(NetworkMessage::NetworkMessageHeader));

					char* aTopic=aBuffer+sizeof(NetworkMessage::NetworkMessageHeader); // ++ v1.5
					char* aBufPtr=aTopic+aNMHeader->topiclen;
					
					NetworkMessage* aNetworkMessage=new NetworkMessage(aBufPtr,aNMHeader->buflen);

					if(aNMHeader->topiclen>0) // ++ v1.5
						aNetworkMessage->setTopic(aTopic,aNMHeader->topiclen); 
	
					if(anHeader.type==MQ_PROXY_UNSOLICITED)
						aNetworkMessage->setUnsolicited();					
					else if(anHeader.type==MQ_PROXY_BROADCAST) //++v1.5
						aNetworkMessage->setBroadcasting();
	
					aNetworkMessage->setSender(getID()); // v1.5
					aNetworkMessage->setRemoteSender(aNMHeader->sender); // v1.5
					aNetworkMessage->setTarget(anHeader.target);				
					aNetworkMessage->setSequenceNumber(aNMHeader->seqnum); // ++  v1.1
					
					if(anHeader.type==MQ_PROXY_BROADCAST)
					{
						broadcast(aNetworkMessage);
						TRACE("Message broadcasted")
					}
					else
					{				
						post(anHeader.target,aNetworkMessage);
						TRACE("Message delivered")
					}
				}
				else if(anHeader.type==MQ_PROXY_LOOKUP_REQUEST)
				{
					TRACE("type==MQ_PROXY_LOOKUP_REQUEST")
					LookupRequestMessage::LookupRequest* aLookup=(LookupRequestMessage::LookupRequest*)aBuffer;
					if(aLookup->namelen > 0xFFFF - sizeof(aLookup))
					{
						WARNING("Buffer overflow detected. Drop connection!")
						break;	
					}
	
					DUMP("Lookup Rx header",(char*)aLookup,sizeof(aLookup));
					const char* aNamePtr=aBuffer+sizeof(LookupRequestMessage::LookupRequest);
					string aName;
					aName.assign(aNamePtr,aLookup->namelen);
					
					MQHANDLE anHandle;
					if(lookup(aName.c_str(),anHandle))
					{
						TRACE("Lookup of " << aName.c_str() << " ok")
						TRACE("Sender=" << aLookup->sender)
						TRACE("Handle=" << anHandle)
						LookupReplyMessage* aMessage=new LookupReplyMessage(aLookup->sender,anHandle); // ++ v1.5
						aMessage->setSender(getID()); // ++ v1.5
						post(aMessage);	// v1.5
					}
					else
					{
						TRACE("Lookup failed")
						TRACE("Sender=" << aLookup->sender)
						LookupReplyMessage* aMessage=new LookupReplyMessage(aLookup->sender); // ++ v1.5
						aMessage->setSender(getID()); // ++ v1.5
						post(aMessage);	// v1.5
					}
				}	
				else if(anHeader.type==MQ_PROXY_LOOKUP_REPLY)
				{
					TRACE("type==MQ_PROXY_LOOKUP_REPLY")
					LookupReplyMessage::LookupReply* aLookup=(LookupReplyMessage::LookupReply*)aBuffer;
					LookupReplyMessage* aReply;
	
					if(aLookup->fail)
						aReply=new LookupReplyMessage();
					else
						aReply=new LookupReplyMessage(*aLookup);				
	
					aReply->setSender(getID());
					post(anHeader.target,aReply);				
					TRACE("Lookup delivered")
				}
				else if(anHeader.type==MQ_PROXY_PING_REQUEST)
				{
					TRACE("type==MQ_PROXY_PING_REQUEST")
					PingRequestMessage::PingRequest* aPing=(PingRequestMessage::PingRequest*)aBuffer;
					PingReplyMessage* aMessage=new PingReplyMessage(aPing->sender);
					aMessage->setSender(getID()); // ++ v1.5
					post(aMessage);	// v1.5
				}
				else if(anHeader.type==MQ_PROXY_PING_REPLY)
				{
					TRACE("type==MQ_PROXY_PING_REPLY")
					PingReplyMessage* aMessage=new PingReplyMessage(0); // ++ v1.5
					aMessage->setSender(getID()); // ++ v1.5
					post(anHeader.target,aMessage); // v1.5
				}
				else
				{
					WARNING("Invalid Rx type. Flush Rx channel.")
					BUFFER((char*)&anHeader,sizeof(header))
					string aBuffer=itsSocket->ReceiveBytes();
					//BUFFER((char*)aBuffer.c_str(),aBuffer.length())
				}				
			}
			else
			{
				WARNING("Invalid sync. Flush Rx channel.")
				BUFFER((char*)&anHeader,sizeof(header))
				string aBuffer=itsSocket->ReceiveBytes();
				//BUFFER((char*)aBuffer.c_str(),aBuffer.length())
			}
		}
		catch(Exception& ex)
		{
			WARNING(ex.getMessage().c_str())			
		}
		catch(...)
		{
			CRITICAL("Unhandled exception")
		}
  	}
  	
  	TRACE("Close socket and stop also Tx tread")

	delete [] aBuffer;			
	Thread::stop(false);
  	itsSocket->Close();
  	
  	WARNING("Connection broken")
  	
	TRACE("MessageProxy::receive - end")
}

Thread MessageProxyFactory::itsMutex("MessageProxyFactoryMutex");

void MessageProxyFactory::ping(const char* theHost, unsigned thePort,
							   MessageQueue* theSourceQueue)
{
	TRACE("MessageFactory::ping(static) - start")
	PingRequestMessage* aMessage=new PingRequestMessage(theSourceQueue->getID());
    MessageProxyFactory::post(theHost,thePort,aMessage,theSourceQueue->getID());
	TRACE("MessageFactory::ping(static) - end")
}

void MessageProxyFactory::lookupAt(const char* theHost, unsigned thePort,
								   const char* theRemoteQueueName,MessageQueue* theSourceQueue)
{
	TRACE("MessageFactory::lookupAt(static) - start")
	LookupRequestMessage* aMessage=new LookupRequestMessage(theRemoteQueueName,theSourceQueue->getID());
    MessageProxyFactory::post(theHost,thePort,aMessage,theSourceQueue->getID());
	TRACE("MessageFactory::lookupAt(static) - end")
}

void MessageProxyFactory::post(const char* theHost, unsigned thePort,Message* theMessage,MQHANDLE theSender)
{
	TRACE("MessageFactory::post(static) - start")
	ostrstream aStream;
	aStream << MESSAGEPROXYHEADER << theHost << "," << thePort << ")" << ends;
	char* aName =  aStream.str();
	TRACE("Proxy name=" << aName)

	MQHANDLE anHandle;
	itsMutex.wait(); // ++ v1.5
	
	if(MessageQueue::lookup(aName,anHandle))
	{
		TRACE("Using existent connection")
		TRACE("Handle=" << anHandle)
		MessageQueue::post(anHandle,theMessage);	
		TRACE("Message sent")
	}
	else
	{
		TRACE("Create new connection")
		Socket* aSocket=NULL;
		MessageProxy* aProxy=NULL;
		
		try
		{
			aSocket=new SocketClient(string(theHost),thePort);
			TRACE("New client connection created")
			aProxy=new MessageProxy(aName,aSocket);
			aProxy->post(theMessage);
			TRACE("Message sent")
			
			char aValue[10];
			ostrstream aStream(aValue,sizeof(aValue));
			aStream << thePort << ends;

			string aMsg=string("Connected to ")+string(theHost)+string(":")+aValue;
			LOG(aMsg.c_str())
		}
		catch(Exception& exc)
		{
			if(theMessage->is("LookupRequestMessage")) Decoupler::deferredPost(theSender,new LookupReplyMessage());	
			if(aProxy!=NULL) delete aProxy;
			else if(aSocket!=NULL) delete aSocket;
			delete[] aName;	
			delete theMessage;
			itsMutex.release();
			string aMsg=string("Fail to create new server connection: ") + exc.getMessage();
			LOG(aMsg.c_str())
			return;
		}
	}

	itsMutex.release();  // ++ v1.5
	delete[] aName;		
	TRACE("MessageFactory::post(static) - end")
}

MessageProxyFactory::MessageProxyFactory(const char* theName,int theSocket)
	  				:Thread(theName), SocketServer(theSocket,MAX_CONNECTIONS),
	  				 itsCount(0), itsPort(theSocket)
{
	TRACE("MessageProxyFactory::MessageProxyFactory - start")
	start();
	TRACE("MessageProxyFactory::MessageProxyFactory - end")
}
	
MessageProxyFactory::~MessageProxyFactory() 
{	
	TRACE("MessageProxyFactory::~MessageProxyFactory - start")
	Close(); //Close socket
	stop(false); //Wait exit of MessageProxyFactory::run  
	TRACE("MessageProxyFactory::~MessageProxyFactory - end")
}

void MessageProxyFactory::run() 
{
	TRACE("MessageProxyFactory::run - start")
	
	while(true)
	{
		TESTCANCEL
		if(Thread::isShuttingDown())
			break;
							
		Socket* aSocket=NULL;
		MessageProxy* aProxy=NULL;		
		try
		{
		    char aValue[10];
		    aSocket=Accept();
		    string anAddr=address();
		    unsigned short aPort=port();

		    ostrstream aMsgStream(aValue,sizeof(aValue));
		    aMsgStream << aPort << ends;

		    string aMsg=string("Connected to ")+anAddr+string(":")+aValue;
		    LOG(aMsg.c_str())
		    itsCount++;
		    ostrstream aStream;
		    aStream << MESSAGEPROXYHEADER << anAddr << "," << aPort << ")" << ends; 
			char* aName=aStream.str();
		    aProxy=new MessageProxy(aName,aSocket);
		    TRACE(aName << " proxy started")
		    delete [] aName;
		    
		    onNewConnection(anAddr,aPort);
		}
		catch(Exception& exc)
		{
			string aMsg=string("Fail to create new server connection: ") + exc.getMessage();
			WARNING(aMsg.c_str())
			
			if(aProxy!=NULL) 
				delete aProxy;
			else if(aSocket!=NULL) 
				delete aSocket;
		}
	}
	
	TRACE("MessageProxyFactory::run - end")		
}

string MessageProxyFactory::getUniqueNetID()
{
	TRACE("MessageProxyFactory::getUniqueNetID - start")		
	string ret;
	vector<NetAdapter>* aList=Socket::getAdapters();	

	string allmacs;
	if(aList!=NULL && aList->size()>0)
	{
		for(vector<NetAdapter>::iterator i = aList->begin(); i < aList->end(); ++i)
		{
			NetAdapter& anAdapter=*i;
			allmacs+=anAdapter.getMAC();
		}	
	}
	
	unsigned int anHash=APHash(allmacs);
	ret+=string((char*)&anHash,sizeof(anHash));	
	
	_TIMEVAL now=Timer::timeExt();
	ret+=string((char*)&now,sizeof(now));	
	
	::srand(Timer::time());
	unsigned int aRand=::rand();
	ret+=string((char*)&aRand,sizeof(aRand));	

	delete aList;
	TRACE("MessageProxyFactory::getUniqueNetID - end")		
	return ret;
}

#ifdef WIN32
unsigned int _mp_thread_proc(void* param) 
{
	TRACE("Start _mp_thread_proc")
	try
	{	
		MessageProxy* tp = (MessageProxy*)param;
		tp->receive();
	}
	catch(...)
	{
		DISPLAY("_mp_thread_proc: Unhandled exception")
	}
	TRACE("End _mp_thread_proc")	
	return 0;
}
#else
void* _mp_thread_proc(void* param) 
{
	TRACE("Start _mp_thread_proc")	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	try
	{	
		MessageProxy* tp = (MessageProxy*)param;
		tp->receive();
	}
	catch(...)
	{
		DISPLAY("_mp_thread_proc: Unhandled exception")	
	}
	TRACE("End _mp_thread_proc")
	pthread_exit(NULL);
	return NULL;
}
#endif
