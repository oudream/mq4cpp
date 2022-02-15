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

#ifndef __ROUTER__
#define __ROUTER__

#include "MessageProxy.h"
#include "Vector.h"
#include "Timer.h"
#include <vector>
using namespace std;
#define MAXSESSIONS 256
#define SESSION_EXPIRATION_TIME 10000

class RemoteRouter : public MessageQueue
{
protected:
	bool itsConnected;
	MQHANDLE itsProxy;	
	MQHANDLE itsServer;
	string itsHost;
	int itsPort; 
	string itsTarget; 

	typedef struct _RoutingSession
	{
		MQHANDLE proxy;	
		MQHANDLE client;
		unsigned short seqnum;
		_TIMEVAL time;		
	} RoutingSession;

	unsigned int itsSeqNum;
	RoutingSession itsSessions[MAXSESSIONS];
	
public:
	RemoteRouter(const char* theName, const char* theHost,int thePort, const char* theTarget);
	virtual ~RemoteRouter();
		 
protected:
	virtual void onMessage(Message* theMessage);
	virtual void onLookup(LookupReplyMessage* theMessage);
	virtual void onWakeup(Wakeup* theMessage);
};

class LocalRouter : public MessageQueue
{
protected:
	MQHANDLE itsServer;

	typedef struct _RoutingSession
	{
		MQHANDLE proxy;	
		MQHANDLE client;
		unsigned short seqnum;	
		_TIMEVAL time;		
	} RoutingSession;

	unsigned int itsSeqNum;
	RoutingSession itsSessions[MAXSESSIONS];

public:
	LocalRouter(const char* theName, const char* theTarget);
	virtual ~LocalRouter();
		 
protected:
	virtual void onMessage(Message* theMessage);
}; 

class Switch : public MessageProxy
{
protected:
	vector<MessageQueue*> itsRouters;
	MessageQueue* itsActiveRouter;
	vector< pair<string,MQHANDLE> > itsMapping;
	vector<string> itsAlias;
	string itsTopic;

	typedef struct _RoutingSession
	{
		MQHANDLE proxy;	
		MQHANDLE client;
		MQHANDLE server;
		unsigned short seqnum;	
		_TIMEVAL time;		
	} RoutingSession;

	unsigned int itsSeqNum;
	RoutingSession itsSessions[MAXSESSIONS];
	
public:
	Switch(const char* theName);
	virtual void addAlias(const char* theName);
	virtual ~Switch();

	virtual MQHANDLE addRouting(const char* theHost,int thePort, const char* theTarget);
	virtual MQHANDLE addRouting(const char* theTarget);
	virtual MQHANDLE addRouting(MessageQueue* theTarget);
	virtual void addRouting(const char* theTopic,MQHANDLE theHandle);
	virtual void removeRouting(MQHANDLE theHandle);
	virtual void resetRouting();
	virtual void activate(MQHANDLE theHandle,const char* theTopic="");
	virtual bool is(const char* theName,MQHANDLE& theID);
	virtual string getConnectionAddress(MQHANDLE theCaller,int& thePort);

protected:
	virtual void onMessage(Message* theMessage);
	virtual void receive() {};
};

#define LOCALHOST "__internal__"
#define LOCALPORT 0

class LocalhostRouter : public MessageProxy
{
public:
	LocalhostRouter();
	virtual ~LocalhostRouter();
	virtual string getConnectionAddress(MQHANDLE theCaller,int& thePort);

protected:
	virtual void onMessage(Message* theMessage);
	virtual void receive() {};
};

#endif

