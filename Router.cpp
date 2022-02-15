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
#include "Router.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;

//#define DISPLAY_TRAFFIC
#ifdef DISPLAY_TRAFFIC
#define DISPLAYC2S { cout << ">"; }
#define DISPLAYS2C { cout << "<"; }
#define DISPLAYB2S { cout << "*"; }
#else
#define DISPLAYC2S
#define DISPLAYS2C
#define DISPLAYB2S
#endif

RemoteRouter::RemoteRouter(const char* theName, const char* theHost,int thePort, const char* theTarget) 
	   :MessageQueue(theName)
{
	TRACE("RemoteRouter::RemoteRouter - start")
	TRACE("Queue name=" << getName())
	itsSeqNum=0;
	itsProxy=0;
	itsServer=0;
	itsHost=theHost;
	itsPort=thePort;
	itsTarget=theTarget;
	itsConnected=false;
	
	for(int i=0; i < MAXSESSIONS; i++)
	{
		itsSessions[i].proxy=0;
		itsSessions[i].client=0;
		itsSessions[i].seqnum=0;
	}
	
	SCHEDULE(this,5000);
	MessageProxyFactory::lookupAt(itsHost.c_str(),itsPort,itsTarget.c_str(),this);			
	TRACE("RemoteRouter::RemoteRouter - end")
}
	
RemoteRouter::~RemoteRouter() 
{
	TRACE("RemoteRouter::~RemoteRouter - start")

	TRACE("RemoteRouter::~RemoteRouter - end")
}

void RemoteRouter::onLookup(LookupReplyMessage* theMessage)
{
	TRACE("RemoteRouter::onLookup - start")
	if(itsConnected==false && !theMessage->isFailed())
	{
		itsServer=theMessage->getHandle();
		itsProxy=theMessage->getSender();
		itsConnected=true;
		LOG("Remote thread lookup ok.")	
	}
	TRACE("RemoteRouter::onLookup - end")
}

void RemoteRouter::onWakeup(Wakeup* theMessage)
{
	TRACE("RemoteRouter::onWakeup - start")
	if(itsConnected==false || (itsConnected==true && !isStillAvailable(itsProxy)))
	{
		itsConnected=false;	
		TRACE("Try to lookup service")
		MessageProxyFactory::lookupAt(itsHost.c_str(),itsPort,itsTarget.c_str(),this);			
	}
	TRACE("RemoteRouter::onWakeup - end")
}

void RemoteRouter::onMessage(Message* theMessage)
{
	TRACE("RemoteRouter::onMessage - start")
	TRACE("Thread name=" << getName())

	try
	{
		if(theMessage->is("Wakeup") && !isShuttingDown())
		{
			TRACE("Call onWakeup")
			onWakeup((Wakeup*)theMessage);
		}
		else if(theMessage->is("LookupReplyMessage") && !isShuttingDown())
		{
			TRACE("Call onLookup")
			onLookup((LookupReplyMessage*)theMessage);
		}
		else if(theMessage->is("NetworkMessage") && itsConnected && !isShuttingDown())
		{
			NetworkMessage* aRequest=(NetworkMessage*)theMessage;

			if(itsProxy==aRequest->getSender() && itsServer==aRequest->getRemoteSender() && !aRequest->isBroadcasting())
			{
				TRACE("Handling message from server")
				DISPLAYS2C

				int anIndex=aRequest->getSequenceNumber() % MAXSESSIONS;
				if(itsSessions[anIndex].proxy!=0 && isStillAvailable(itsSessions[anIndex].proxy))
				{
					_TIMEVAL current=Timer::timeExt();
					long elapsed=Timer::subtractMillisecs(&itsSessions[anIndex].time,&current);

					if(elapsed < SESSION_EXPIRATION_TIME)
					{
						NetworkMessage* aReply=(NetworkMessage*)aRequest->clone();
						aReply->setSender(getID());
						aReply->setRemoteSender(0);
						aReply->setTarget(itsSessions[anIndex].client);
						aReply->setSequenceNumber(itsSessions[anIndex].seqnum);
						post(itsSessions[anIndex].proxy,aReply); 
					}

					itsSessions[anIndex].proxy=0;
					itsSessions[anIndex].client=0;
					itsSessions[anIndex].seqnum=0;
				}
			}
			else if(!aRequest->isBroadcasting() && !isShuttingDown())
			{
				TRACE("Handling message from client")
				if(itsConnected==true)
				{
					DISPLAYC2S
					
					unsigned short anIndex=itsSeqNum % MAXSESSIONS;					
					itsSessions[anIndex].proxy=aRequest->getSender();
					itsSessions[anIndex].client=aRequest->getRemoteSender();
					itsSessions[anIndex].seqnum=aRequest->getSequenceNumber();
					itsSessions[anIndex].time=Timer::timeExt();

					NetworkMessage* aNewRequest=(NetworkMessage*)aRequest->clone();
					aNewRequest->setSender(getID());
					aNewRequest->setRemoteSender(0);
					aNewRequest->setTarget(itsServer);
					aNewRequest->setSequenceNumber(itsSeqNum);
					post(itsProxy,aNewRequest);

					itsSeqNum++;
				}			
			}
		}
	}
	catch(Exception& ex)
	{
		DISPLAY("RemoteRouter::onMessage(" << getName() << ") : " << ex.getMessage().c_str())
	}
	catch(...)
	{		
		DISPLAY("RemoteRouter::onMessage(" << getName() << ") : Unhandled exception")
	}

	TRACE("RemoteRouter::onMessage - end")
}

LocalRouter::LocalRouter(const char* theName, const char* theTarget) 
	   :MessageQueue(theName)
{
	TRACE("LocalRouter::LocalRouter - start")
	TRACE("Queue name=" << getName())
	itsSeqNum=0;
	if(!lookup(theTarget,itsServer))
		throw ThreadException("Lookup of local service failed");
	
	for(int i=0; i < MAXSESSIONS; i++)
	{
		itsSessions[i].proxy=0;
		itsSessions[i].client=0;
		itsSessions[i].seqnum=0;
	}

	TRACE("LocalRouter::LocalRouter - end")
}
	
LocalRouter::~LocalRouter() 
{
	TRACE("LocalRouter::~LocalRouter - start")

	TRACE("LocalRouter::~LocalRouter - end")
}

void LocalRouter::onMessage(Message* theMessage)
{
	TRACE("LocalRouter::onMessage - start")
	TRACE("Thread name=" << getName())

	try
	{
		if(theMessage->is("NetworkMessage") && !isShuttingDown())
		{
			NetworkMessage* aRequest=(NetworkMessage*)theMessage;

			if(itsServer==aRequest->getSender() && aRequest->getRemoteSender()==0 && !aRequest->isBroadcasting())
			{
				TRACE("Handling message from server")	
				int anIndex=aRequest->getSequenceNumber() % MAXSESSIONS;
				if(itsSessions[anIndex].proxy!=0 && isStillAvailable(itsSessions[anIndex].proxy))
				{
					DISPLAYS2C

					_TIMEVAL current=Timer::timeExt();
					long elapsed=Timer::subtractMillisecs(&itsSessions[anIndex].time,&current);

					if(elapsed < SESSION_EXPIRATION_TIME)
					{
						NetworkMessage* aReply=(NetworkMessage*)aRequest->clone();
						aReply->setSender(getID());
						aReply->setRemoteSender(0);
						aReply->setTarget(itsSessions[anIndex].client);
						aReply->setSequenceNumber(itsSessions[anIndex].seqnum);
						post(itsSessions[anIndex].proxy,aReply); 
					}

					itsSessions[anIndex].proxy=0;
					itsSessions[anIndex].client=0;
					itsSessions[anIndex].seqnum=0;
				}
			}
			else if (!aRequest->isBroadcasting() && !isShuttingDown())
			{
				TRACE("Handling message from client")
				DISPLAYC2S

				unsigned short anIndex=itsSeqNum % MAXSESSIONS;					
				itsSessions[anIndex].proxy=aRequest->getSender();
				itsSessions[anIndex].client=aRequest->getRemoteSender();
				itsSessions[anIndex].seqnum=aRequest->getSequenceNumber();
				itsSessions[anIndex].time=Timer::timeExt();

				NetworkMessage* aNewRequest=(NetworkMessage*)aRequest->clone();
				aNewRequest->setSender(getID());
				aNewRequest->setRemoteSender(getID());
				aNewRequest->setTarget(itsServer);
				aNewRequest->setSequenceNumber(itsSeqNum);
				post(itsServer,aNewRequest);

				itsSeqNum++;
			}
		}
	}
	catch(Exception& ex)
	{
		DISPLAY("LocalRouter::onMessage(" << getName() << ") : " << ex.getMessage().c_str())
	}
	catch(...)
	{		
		DISPLAY("LocalRouter::onMessage(" << getName() << ") : Unhandled exception")
	}

	TRACE("LocalRouter::onMessage - end")
}

Switch::Switch(const char* theName) : MessageProxy(theName)  
{
	TRACE("Switch::Switch - start")
	itsActiveRouter=NULL;
	itsSeqNum=0;
	
	for(int i=0; i < MAXSESSIONS; i++)
	{
		itsSessions[i].proxy=0;
		itsSessions[i].client=0;
		itsSessions[i].seqnum=0;
	}
	
	TRACE("Switch::Switch - end")	
}

Switch::~Switch()
{
	TRACE("Switch::~Switch - start")
	if(!isShuttingDown())
	{
		for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
		{
			MessageQueue* aQueue=*i;
			aQueue->shutdown(); 
		}
	}		
	TRACE("Switch::~Switch - end")	
}

MQHANDLE Switch::addRouting(const char* theHost,int thePort, const char* theTarget)
{
	TRACE("Switch::addRouting - start")
	wait();
	string aName="RemoteRouter("+string(getName())+","+string(theHost)+","+string(theTarget)+")";

	bool found=false;
	MQHANDLE anHandle=0;
	if(itsRouters.size()>0)
	{
		for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
		{
			MessageQueue* aQueue=*i;
			if(aName.compare(aQueue->getName())==0)
			{
				found=true;
				anHandle=aQueue->getID();
				break;
			}
		}
	}

	if(!found)
	{
		RemoteRouter* aRouter=new RemoteRouter(aName.c_str(),theHost,thePort,theTarget);
		itsRouters.push_back(aRouter);	
		anHandle=aRouter->getID();
		if(itsActiveRouter==NULL) itsActiveRouter=aRouter;
	}

	release();	
	TRACE("Switch::addRouting - end")	
	return anHandle;
}

MQHANDLE Switch::addRouting(const char* theTarget)
{
	TRACE("Switch::addRouting - start")
	wait();
	string aName="LocalRouter("+string(getName())+","+string(theTarget)+")";

	bool found=false;
	MQHANDLE anHandle=0;
	if(itsRouters.size()>0)
	{
		for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
		{
			MessageQueue* aQueue=*i;
			if(aName.compare(aQueue->getName())==0)
			{
				found=true;
				anHandle=aQueue->getID();
				break;
			}
		}
	}

	if(!found)
	{
		LocalRouter* aRouter=new LocalRouter(aName.c_str(),theTarget);
		itsRouters.push_back(aRouter);	
		anHandle=aRouter->getID();
		if(itsActiveRouter==NULL) itsActiveRouter=aRouter;
	}
	release();	
	TRACE("Switch::addRouting - end")	
	return anHandle;	
}

MQHANDLE Switch::addRouting(MessageQueue* theTarget)
{
	TRACE("Switch::addRouting - start")
	wait();
	MQHANDLE anHandle=0;
	if(theTarget!=NULL)
	{
		itsRouters.push_back(theTarget);	
		anHandle=theTarget->getID();
		if(itsActiveRouter==NULL) itsActiveRouter=theTarget;
	}
	release();	
	TRACE("Switch::addRouting - end")	
	return anHandle;	
}

void Switch::addRouting(const char* theTopic,MQHANDLE theHandle)
{
	TRACE("Switch::addRouting - start")
	wait();

	bool found=false;
	if(itsMapping.size()>0)
	{
		for(vector< pair<string,MQHANDLE> >::iterator i = itsMapping.begin(); i < itsMapping.end(); ++i)
		{
			pair<string,MQHANDLE> aPair=*i;
			if(aPair.first.compare(theTopic)==0 && aPair.second==theHandle)
			{
				found=true;
				break;
			}
		}
	}
	
	if(!found && itsRouters.size()>0)
	{
		for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
		{
			MessageQueue* aQueue=*i;
			if(aQueue->getID()==theHandle)
			{
				pair<string,MQHANDLE> aPair;
				aPair.first=theTopic;
				aPair.second=theHandle;
				itsMapping.push_back(aPair);
				TRACE("Mapping rule added")
				break;
			}
		}
	}
	release();	
	TRACE("Switch::addRouting - end")	
}

void Switch::resetRouting()
{
	TRACE("Switch::resetRouting - start")
	wait();
	if(!isShuttingDown())
	{
		if(itsRouters.size()>0)
		{
			for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
			{
				MessageQueue* aQueue=*i;
				aQueue->shutdown();
			}		
		}
	}

	itsRouters.clear();
	itsActiveRouter=NULL;
	itsMapping.clear();
	release();	
	TRACE("Switch::resetRouting - end")	
}

void Switch::removeRouting(MQHANDLE theHandle)
{
	TRACE("Switch::removeRouting - start")
	wait();
	if(!isShuttingDown())
	{
		for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
		{
			MessageQueue* aQueue=*i;
			if(aQueue->getID()==theHandle)
			{
				aQueue->shutdown();
				itsRouters.erase(i);
				if(itsActiveRouter==aQueue) itsActiveRouter=NULL;
				TRACE("Router removed")
				break;
			}
		}
	}		
	release();	
	TRACE("Switch::removeRouting - end")	
}

void Switch::activate(MQHANDLE theHandle,const char* theTopic)
{
	TRACE("Switch::activate - start")
	wait();
	for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
	{
		MessageQueue* aQueue=*i;
		if(aQueue->getID()==theHandle)
		{
			itsActiveRouter=aQueue;
			itsTopic=theTopic;
			TRACE("Router activated")
			break;
		}
	}		
	release();	
	TRACE("Switch::activate - end")	
}

void Switch::addAlias(const char* theName)
{
	TRACE("Switch::addAlias - start")
	itsAlias.push_back(theName);	
	TRACE("Switch::addAlias - end")
}

bool Switch::is(const char* theName,MQHANDLE& theID)
{
	TRACE("Switch::is - start")
	TRACE("Its name=" << getName() << " Requested name=" << theName)
	bool ret=false;
	if(Thread::is(theName))
	{
		theID=itsID;
		ret=true;
	}	
	else if(itsAlias.size()>0)
	{
		for(vector<string>::iterator i = itsAlias.begin(); i < itsAlias.end(); ++i)
		{
			string& aString=*i;
			if(aString.compare(theName)==0)
			{
				theID=itsID;
				ret=true;
				break;
			}
		}		
	}
	TRACE(((ret) ? "Found" : "Not found"))
	TRACE("Switch::is - end")
	return ret;
}

void Switch::onMessage(Message* theMessage)
{
	TRACE("Switch::onMessage - start")

	if(theMessage->is("NetworkMessage") && !isShuttingDown())
	{
		NetworkMessage* aRequest=(NetworkMessage*)theMessage;
		
		bool found=false;
		if(itsRouters.size()>0)
		{
			for(vector<MessageQueue*>::iterator i = itsRouters.begin(); i < itsRouters.end(); ++i)
			{
				MessageQueue* aQueue=*i;
				if(aQueue->getID()==aRequest->getSender())
				{
					found=true;
					break;
				}
			}		
		}
		
		if(found && aRequest->getRemoteSender()==0 && !aRequest->isBroadcasting())
		{
			TRACE("Handling message from server")	
			int anIndex=aRequest->getSequenceNumber() % MAXSESSIONS;
			if(itsSessions[anIndex].proxy!=0 && isStillAvailable(itsSessions[anIndex].proxy))
			{
				_TIMEVAL current=Timer::timeExt();
				long elapsed=Timer::subtractMillisecs(&itsSessions[anIndex].time,&current);

				if(elapsed < SESSION_EXPIRATION_TIME)
				{
					NetworkMessage* aReply=(NetworkMessage*)aRequest->clone();
					aReply->setSender(getID());
					aReply->setRemoteSender(0);
					aReply->setTarget(itsSessions[anIndex].client);
					aReply->setSequenceNumber(itsSessions[anIndex].seqnum);
					post(itsSessions[anIndex].proxy,aReply); 
				}

				itsSessions[anIndex].proxy=0;
				itsSessions[anIndex].client=0;
				itsSessions[anIndex].server=0;
				itsSessions[anIndex].seqnum=0;
			}
		}
		else if (!found && !aRequest->isBroadcasting() && !isShuttingDown())
		{
			TRACE("Handling message from client")	

			bool fired=false;
			if(itsMapping.size()>0)
			{
				for(vector< pair<string,MQHANDLE> >::iterator i = itsMapping.begin(); i < itsMapping.end(); ++i)
				{
					pair<string,MQHANDLE> aPair=*i;
					
					if(aPair.first.compare(aRequest->getTopic())==0)
					{
						unsigned short anIndex=itsSeqNum % MAXSESSIONS;					
						itsSessions[anIndex].proxy=aRequest->getSender();
						itsSessions[anIndex].client=aRequest->getRemoteSender();
						itsSessions[anIndex].seqnum=aRequest->getSequenceNumber();
						itsSessions[anIndex].server=aPair.second;
						itsSessions[anIndex].time=Timer::timeExt();
		
						NetworkMessage* aNewRequest=(NetworkMessage*)aRequest->clone();
						aNewRequest->setSender(getID());
						aNewRequest->setRemoteSender(getID());
						aNewRequest->setTarget(aPair.second);
						aNewRequest->setSequenceNumber(itsSeqNum);
						post(aPair.second,aNewRequest);
						itsSeqNum++;
						fired=true;
						TRACE("Sent message with topic=" << aRequest->getTopic())
						break;
					}
				}		
			}
			
			if(!fired && itsActiveRouter!=NULL)
			{
	
				unsigned short anIndex=itsSeqNum % MAXSESSIONS;					
				itsSessions[anIndex].proxy=aRequest->getSender();
				itsSessions[anIndex].client=aRequest->getRemoteSender();
				itsSessions[anIndex].server=itsActiveRouter->getID();
				itsSessions[anIndex].seqnum=aRequest->getSequenceNumber();
				itsSessions[anIndex].time=Timer::timeExt();

				NetworkMessage* aNewRequest=(NetworkMessage*)aRequest->clone();
				if(itsTopic.size()>0) aNewRequest->setTopic(itsTopic);
				aNewRequest->setSender(getID());
				aNewRequest->setRemoteSender(getID());
				aNewRequest->setTarget(itsActiveRouter->getID());
				aNewRequest->setSequenceNumber(itsSeqNum);
				itsActiveRouter->post(aNewRequest);
				itsSeqNum++;
			}
		}
	}
	
	TRACE("Switch::onMessage - end")
}

string Switch::getConnectionAddress(MQHANDLE theCaller,int& thePort)
{
	TRACE("Switch::getConnectionAddress - start")
	string addr=LOCALHOST;
	thePort=LOCALPORT;

	if(!isShuttingDown())
	{
		for(unsigned i=0; i < MAXSESSIONS; i++)
		{
			if(itsSessions[i].server==theCaller)
			{
				MessageQueue* aQueue=lookup(itsSessions[i].proxy);
				if(aQueue!=NULL)
				{	
					if(string(aQueue->getName()).compare(MESSAGEPROXYHEADER)>=0)
					{
						MessageProxy* aProxy=(MessageProxy*)aQueue;
						addr=aProxy->getConnectionAddress(getID(),thePort);					
						break;	
					}
				}
			}
		}
	}
	
	TRACE("Switch::getConnectionAddress - end")
	return addr;
}

LocalhostRouter::LocalhostRouter() 
			    :MessageProxy((string(MESSAGEPROXYHEADER)+string(LOCALHOST)+",0)").c_str())
{
	TRACE("LocalhostRouter::LocalhostRouter - start")

	TRACE("LocalhostRouter::LocalhostRouter - end")		
}

LocalhostRouter::~LocalhostRouter()
{
	TRACE("LocalhostRouter::~LocalhostRouter - start")

	TRACE("LocalhostRouter::~LocalhostRouter - end")			
}

void LocalhostRouter::onMessage(Message* theMessage)
{
	TRACE("LocalhostRouter::onMessage - start")
	
	if(theMessage->is("NetworkMessage") && !isShuttingDown())
	{
		NetworkMessage* aMessage=(NetworkMessage*)theMessage;
		if(!aMessage->isBroadcasting())
		{				
			NetworkMessage* aNetworkMessage=(NetworkMessage*)aMessage->clone();
			aNetworkMessage->setSender(getID());
			aNetworkMessage->setRemoteSender(aMessage->getSender());
			post(aMessage->getTarget(),aNetworkMessage);
			TRACE("Message delivered")
		}	
	}
	else if(theMessage->is("LookupRequestMessage") && !isShuttingDown())
	{
		LookupRequestMessage* aMessage=(LookupRequestMessage*)theMessage;
					
		MQHANDLE anHandle;
		if(lookup(aMessage->getTarget().c_str(),anHandle))
		{
			LookupReplyMessage* aReply=new LookupReplyMessage(0,anHandle);
			aReply->setSender(getID());
			post(aMessage->getSender(),aReply);				
		}
		else
		{
			LookupReplyMessage* aReply=new LookupReplyMessage();
			aReply->setSender(getID());
			post(aMessage->getSender(),aReply);				
		}		
	}
	else if(theMessage->is("PingRequestMessage") && !isShuttingDown())
	{
		PingRequestMessage* aMessage=(PingRequestMessage*)theMessage;
		PingReplyMessage* aNewMessage=new PingReplyMessage(aMessage->getSender());
		aMessage->setSender(getID()); 
		post(aMessage->getSender(),aNewMessage); 
	}
	
	TRACE("LocalhostRouter::onMessage - end")
}

string LocalhostRouter::getConnectionAddress(MQHANDLE theHandle,int& thePort)
{
	TRACE("LocalhostRouter::getConnectionAddress - start")
	thePort=LOCALPORT;
	TRACE("LocalhostRouter::getConnectionAddress - end")
	return LOCALHOST;
}


