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

#ifndef __MESSAGEQUEUE__
#define __MESSAGEQUEUE__

#include "LinkedList.h"
#include "Thread.h"
#include "Registry.h"
#include <iostream>
#include <string>
using namespace std;

class Message
{
protected:
	string itsClassName; // v1.5
	MQHANDLE itsSender; // ++ v1.5

public:	
	Message(Message& o) // ++ v1.5
	   : itsClassName(o.itsClassName), itsSender(o.itsSender) {};
	Message(const char* theClassName) 
	   : itsClassName(theClassName), itsSender(0) {};

	virtual ~Message() {};
	virtual Message* clone() { return NULL; }; // ++ v1.5

	const char* getClass() { return itsClassName.c_str(); }; // v1.5
	virtual bool is(const char* theName);

	virtual void toStream(ostream& theStream) { theStream << itsClassName; };
	virtual string toString() { return ""; };

	void setSender(MQHANDLE theHandle) { itsSender=theHandle; };
	MQHANDLE getSender() { return itsSender; };
};

class MessageQueue : public Thread, protected LinkedList
{
protected:
	static Registry* itsRegistry;
	MQHANDLE itsID;	
	
public:	
	MessageQueue(const char* theThreadName);
	virtual ~MessageQueue();
	MQHANDLE getID() { return itsID; };
	void setID(MQHANDLE theID) { itsID=theID; };
	void flush();
	virtual void post(Message* theMessage);	
	virtual bool is(const char* theName,MQHANDLE& theID);
	virtual void shutdown();

	static void post(MQHANDLE theTarget,Message* theMessage);
	static void broadcast(Message* theMessage);
	static bool lookup(const char* theName,MQHANDLE& theID);
	static MessageQueue* lookup(MQHANDLE theID);	
	static bool isStillAvailable(MQHANDLE theTarget);
	static void waitForCompletion();
	static void dump();
	
	friend class Registry;		

protected:
	virtual void run();
	virtual void deleteObject(void* theObject) { delete (Message*)theObject; }; //++ v1.5
	virtual void onMessage(Message* theMessage)=0;
	virtual void onException(Exception& ex); 
	static void add(MessageQueue* theQueue);
	static void remove(MessageQueue* theQueue);	
};

#define STOPREGISTRY() \
	MessageQueue::waitForCompletion();

class Decoupler : protected MessageQueue
{
protected:
	class DeferredMessage : public Message
	{
	protected:
		MQHANDLE itsTarget;
		Message* itsMessage;
		
	public:
		DeferredMessage(MQHANDLE theTarget,Message* theMessage) 
		   : Message("DeferredMessage"), itsTarget(theTarget), itsMessage(theMessage) {};

		~DeferredMessage() {};
		MQHANDLE getTarget() { return itsTarget; };
		Message* getMessage() { return itsMessage; };	
	};

	static Decoupler* itsDefaultDecoupler;

public:
	Decoupler(const char* theThreadName) : MessageQueue(theThreadName) {};
	virtual ~Decoupler() {};
	virtual void post(MQHANDLE theTarget,Message* theMessage);
	static void deferredPost(MQHANDLE theTarget,Message* theMessage);	
	static void deferredBroadcast(Message* theMessage);	

protected:
	virtual void onMessage(Message* theMessage);
};

#ifndef WIN32 
#define TESTCANCEL \
	if(itsRunningFlag==false) break; \
	pthread_testcancel(); 
#else
#define TESTCANCEL \
	if(itsRunningFlag==false) break; 
#endif

#endif

