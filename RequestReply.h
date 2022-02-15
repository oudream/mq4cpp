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

#ifndef __REQUESTREPLY__
#define __REQUESTREPLY__

#include "MessageProxy.h"
#include <vector>
using namespace std;

class Client : public Observer
{
protected:
	bool itsConnected;
	MQHANDLE itsProxy;	
	MQHANDLE itsServer;
	unsigned short itsMsgCnt;
	string itsHost;
	int itsPort; 
	string itsTarget; //v1.5
	NetworkMessage* itsMessage;
	unsigned long itsSendTime;
	int itsRetryCount; // ++ v1.2
	string itsTopic;

	typedef struct FailoverEntryStruct
	{
		string host;
		int port;
	} FailoverEntry; 

	std::vector<FailoverEntry*> itsFailoverList;
	unsigned itsFailoverCnt;

public:
	Client(const char* theName, const char* theTarget);
	Client(const char* theName, const char* theHost,int thePort, const char* theTarget);
	virtual ~Client();
	virtual void addFailoverHost(char* theHost,int thePort);
	virtual bool sendMessage(string theBuffer);
	virtual bool test(const char* theHost,int thePort, const char* theTarget);
	virtual bool isConnected();
	virtual void setTopic(const char* theTopic);
		 
protected:
	virtual bool send(string theBuffer); 

	virtual void onLookup(LookupReplyMessage* theMessage);
	virtual NetworkMessage* onRequest(NetworkMessage* theMessage);
	virtual void onWakeup(Wakeup* theMessage);

	virtual void success(string theBuffer)=0;
	virtual void fail(string theError)=0;
	
	virtual void lookup(bool findHost=false);
	virtual void postToProxy();
	virtual void reset();		
};

class Server : public Observer
{
public:
	Server(const char* theName);
	virtual ~Server();

protected:
	virtual NetworkMessage* onRequest(NetworkMessage* theMessage);
	virtual string service(string theBuffer)=0;
};

#endif
