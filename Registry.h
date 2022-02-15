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

#ifndef __REGISTRY__
#define __REGISTRY__

#include "Vector.h"
#include "LinkedList.h"
#include "Thread.h"

class MessageQueue;
class Message;
typedef unsigned short MQHANDLE;

class Registry : protected Vector, protected LinkedList, protected Thread
{
protected:
	enum Action { REMOVE, BROADCAST, LOOKUP, LOOKUP1, GARBAGE_COLLECTION, DUMP } itsAction; 
	MessageQueue* itsMessageQueue;
	Message* itsMessage;
	string itsQueueToLookup;
	MQHANDLE itsFoundID;
	MQHANDLE itsIDToFind;
	bool itsFoundFlag;
	unsigned itsNextHandleAvailable; //++ v1.1

public:	
	Registry(const char* theName);		
	~Registry();
	void add(MessageQueue* theQueue);
	void remove(MessageQueue* theTarget);
	void post(MQHANDLE theTarget,Message* theMessage);
	void broadcast(Message* theMessage);
	bool lookup(const char* theName,MQHANDLE& theID);
	bool isStillAvailable(MQHANDLE theTarget);
	MessageQueue* lookup(MQHANDLE theID);
	void dump();
	
protected:
	virtual void run();
	virtual bool onIteration(LinkedElement* theElement);
	virtual void deleteObject(void* theObject); //++ v1.5
	virtual MQHANDLE findID();
};

#endif
