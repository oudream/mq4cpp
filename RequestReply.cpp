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

#define SILENT
#include "RequestReply.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;
#define REMOTE_OK "OK:"
#define REMOTE_EXCEPTION "EXCEPTION:"
#define REMOTE_TIMEOUT 5
#define RETRYMAX 5
#define RETRYLOOKUP 3

Client::Client(const char* theName, const char* theTarget) 
	   :Observer(theName)
{
	TRACE("Client::Client - start")
	TRACE("Queue name=" << getName())
	itsProxy=0;
	itsServer=0;
	itsMsgCnt=0;
	itsHost.empty();
	itsPort=0;
	itsTarget=theTarget;
	itsConnected=false;
	itsMessage=NULL;
	itsSendTime=0;
	itsFailoverCnt=0;
	itsRetryCount=0;

	bool res=MessageQueue::lookup(theTarget,itsProxy);
	if(!res)
		throw ThreadException("Local service not started");

	itsServer=itsProxy;
	itsConnected=true;

	SCHEDULE(this,500);
	TRACE("Client::Client - end")
}

Client::Client(const char* theName, const char* theHost,int thePort, const char* theTarget) 
	   :Observer(theName)
{
	TRACE("Client::Client - start")
	TRACE("Queue name=" << getName())
	itsProxy=0;
	itsServer=0;
	itsMsgCnt=0;
	itsHost=theHost;
	itsPort=thePort;
	itsTarget=theTarget;
	itsConnected=false;
	itsMessage=NULL;
	itsSendTime=0;
	itsFailoverCnt=0;
	itsRetryCount=0; 
	SCHEDULE(this,500);
	lookup();
	TRACE("Client::Client - end")
}
	
Client::~Client() 
{
	TRACE("Client::~Client - start")
	if(itsMessage!=NULL)
		delete itsMessage;

	for (vector<FailoverEntry*>::iterator i = itsFailoverList.begin(); i < itsFailoverList.end(); ++i)
		delete *i;	
	itsFailoverList.clear();	
	TRACE("Client::~Client - end")
}

bool Client::test(const char* theHost,int thePort, const char* theTarget) 
{
	TRACE("Client::test - start")
	bool ret=false;
	wait();
	ret=(itsHost.compare(theHost)==0)&&(thePort==itsPort)&&(itsTarget.compare(theTarget)==0); 
	release();
	TRACE("Client::test - end")
	return ret;
}

bool Client::isConnected() 
{
	wait();
	TRACE("Client::isConnected - start")
	bool ret=false;

	if(itsConnected==false && itsProxy==0) // Never connected
		ret=true; // Allow the client to estabilish the connection
	else if(itsConnected==true && isStillAvailable(itsProxy)) // Continue to be connected
		ret=true; 
	else
		ret=false; // Connection lost

	release();
	TRACE("Client::isConnected - end")
	return ret; 
}

void Client::setTopic(const char* theTopic) 
{
	wait();
	TRACE("Client::setTopic - start")
	itsTopic=theTopic;
	release();
	TRACE("Client::setTopic - end")
}

void Client::addFailoverHost(char* theHost,int thePort)
{
	TRACE("Client::addFailoverHost - start")
	wait(); //++v1.4
	FailoverEntry* anEntry=new FailoverEntry;
	anEntry->host=theHost;
	anEntry->port=thePort;
	itsFailoverList.push_back(anEntry);
	release(); //++v1.4
	TRACE("Client::addFailoverHost - end")
}

void Client::lookup(bool findHost)
{
	TRACE("Client::lookup - start")
	itsRetryCount=0;

	if(itsFailoverList.empty())
	{
		TRACE("Start lookup default host")
		if(itsHost.size()==0)
		{
			bool res=MessageQueue::lookup(itsTarget.c_str(),itsProxy);
			if(res)
			{
				itsServer=itsProxy;
				itsConnected=true;
			}
		}
		else
		{	
			MessageProxyFactory::lookupAt(itsHost.c_str(),itsPort,itsTarget.c_str(),this);
		}
	}
	else
	{
		if(findHost==true)
		{
			++itsFailoverCnt;
			if(itsFailoverCnt > itsFailoverList.size())
				itsFailoverCnt=0;
		}
		
		if(itsFailoverCnt==0)
		{
			TRACE("Start lookup default host")
			if(itsHost.size()==0)
			{
				bool res=MessageQueue::lookup(itsTarget.c_str(),itsProxy);
				if(res)
				{
					itsServer=itsProxy;
					itsConnected=true;
				}
			}
			else
			{	
				MessageProxyFactory::lookupAt(itsHost.c_str(),itsPort,itsTarget.c_str(),this);
			}
		}
		else
		{
			WARNING("Start to lookup an alternative host")
			FailoverEntry* anEntry=itsFailoverList[itsFailoverCnt-1];
			MessageProxyFactory::lookupAt(anEntry->host.c_str(),anEntry->port,itsTarget.c_str(),this);						
		}
	}
	TRACE("Client::lookup - end")
}

void Client::onLookup(LookupReplyMessage* theMessage)
{
	TRACE("Client::onLookup - start")
	itsRetryCount=0;
	
	if(itsConnected==false && !theMessage->isFailed())
	{
		itsRetryCount=0;		
		itsServer=theMessage->getHandle();
		itsProxy=theMessage->getSender();
		itsConnected=true;
		LOG("Remote thread lookup ok.")	

		if(itsMessage!=NULL)
		{
			LOG("Transmition of queued message")	
			postToProxy();
		}
	}
	TRACE("Client::onLookup - end")
}

void Client::onWakeup(Wakeup* theMessage)
{
	TRACE("Client::onWakeup - start")
	if(itsConnected==false || (itsConnected==true && !isStillAvailable(itsProxy)))
	{
		itsConnected=false;	
		TRACE("Retry count=" << itsRetryCount)
		if(++itsRetryCount>RETRYMAX) // ++ v1.2
		{
			WARNING("Lost peer connection")
			if(itsMessage!=NULL)
			{	
				reset();
				fail("Lost connection");
			}
			itsRetryCount=0;
		}
		else if(itsRetryCount>RETRYLOOKUP) // ++ v1.4
		{
			TRACE("Try to lookup service")
			lookup(true);
		}	
	}
	else if(itsMessage!=NULL && (Timer::time() - itsSendTime > REMOTE_TIMEOUT))
	{
		if(++itsRetryCount>RETRYMAX) // ++ v1.2
		{
			WARNING("Peer timeout")	
			reset();
			fail("Timeout");
		}
		else
		{
			WARNING("Try to retransmit last message")	
			postToProxy();
		}
	}
	TRACE("Client::onWakeup - end")
}

void Client::postToProxy()
{
	TRACE("Client::postToProxy - start")
	if(itsMessage!=NULL) // ++ v1.5
	{
		NetworkMessage* aMessage=(NetworkMessage*)itsMessage->clone(); //++ v1.5
		aMessage->setSender(getID());
		aMessage->setTarget(itsServer);
		aMessage->setTopic(itsTopic);
		itsSendTime=Timer::time();
		post(itsProxy,aMessage);
	}
	TRACE("Client::postToProxy - end")
}

bool Client::sendMessage(string theBuffer) //++v1.4
{
	TRACE("Client::sendMessage - start")
	wait();
	bool ret=send(theBuffer);
	release();
	TRACE("Client::sendMessage - end")
	return ret;
}

bool Client::send(string theBuffer)
{
	TRACE("Client::send - start")
	bool ret=false;
	if(itsMessage==NULL)
	{
		itsMessage=new NetworkMessage(theBuffer);
		itsMessage->setSender(getID());
		itsMessage->setSequenceNumber(itsMsgCnt);
		itsMessage->setTopic(itsTopic);

		if(itsConnected==true && isStillAvailable(itsProxy))
		{
			TRACE("Already connected. Immediate posting.")
			postToProxy();
		}
		
		ret=true;
	}
	else
	{
		WARNING("Client::send : overlaying request during transmition")	
	}
	TRACE("Client::send - end")
	return ret;
}

NetworkMessage* Client::onRequest(NetworkMessage* theMessage)
{
	TRACE("Client::onRequest - start")
	if(theMessage->getSequenceNumber()==itsMsgCnt)
	{
		reset(); // ++ v1.2		
		string response=theMessage->get();
		if(response.substr(0,sizeof(REMOTE_OK)-1).compare(REMOTE_OK)==0)
		{
			TRACE("Service OK")
			delete itsMessage;
			itsMessage=NULL;
			success(response.substr(sizeof(REMOTE_OK)-1,string::npos));
		}
		else if(response.substr(0,sizeof(REMOTE_EXCEPTION)-1).compare(REMOTE_EXCEPTION)==0)
		{
			WARNING((string("Service Error/Exception='")+ response + string("'")).c_str())
			delete itsMessage;
			itsMessage=NULL;
			fail(response.substr(sizeof(REMOTE_EXCEPTION)-1,string::npos));
		}
		else
		{
			WARNING("Client::onRequest: skipped message with bad message header")	
		}				
	}
	else
	{
		WARNING("Client::onRequest: skipped message with bad sequence number")	
	}	
	TRACE("Client::onRequest - end")
	return NULL; // No reply
}

void Client::reset() // ++ v1.2
{
	TRACE("Client::reset - start")
	delete itsMessage;
	itsMessage=NULL;
	itsSendTime=0;
	itsRetryCount=0;	
	itsMsgCnt++;	
	TRACE("Client::reset - end")
}

Server::Server(const char* theName) : Observer(theName)
{
	TRACE("Server::Server - start")

	TRACE("Server::Server - end")
}

Server::~Server()
{
	TRACE("Server::~Server - start")

	TRACE("Server::~Server - end")
}

NetworkMessage* Server::onRequest(NetworkMessage* theMessage)
{
	TRACE("Server::onRequest - start")

	NetworkMessage* aMessage=NULL;
	
	try
	{
		TRACE("Service completed with success")
		string aBuffer=string(REMOTE_OK) + service(theMessage->get());
		aMessage=new NetworkMessage(aBuffer);
	}
	catch(Exception& exc)
	{
		WARNING((string("Exception=") +  exc.getMessage()).c_str())
		ostrstream aStream;
		aStream << REMOTE_EXCEPTION << exc.getMessage().c_str() << ends;
		char* aString=aStream.str();
		int aLen=strlen(aString)+1;
		aMessage=new NetworkMessage(aString,aLen);
		delete [] aString;
	}
	catch(...)
	{
		CRITICAL("Service returns unhandled exception")
		ostrstream aStream;
		aStream << REMOTE_EXCEPTION << "Unhandled exception" << ends;
		char* aString=aStream.str();
		int aLen=strlen(aString)+1;
		aMessage=new NetworkMessage(aString,aLen);
		delete [] aString;
	}
	TRACE("Server::onRequest - end")
	return aMessage; // Send reply message
}

