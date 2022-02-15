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
#include "Trace.h"
#include "MessageQueue.h"
#include "Logger.h"

Registry::Registry(const char* theName) : Thread(theName) 
{
	TRACE("Registry::Registry - start")
	itsNextHandleAvailable=1; //v1.5
	start();
	setPriority(Thread::P_LOWEST);
	TRACE("Registry::Registry - end")
}
		
Registry::~Registry()
{
	TRACE("Registry::~Registry - start")
	stop(false);	
	free();
	TRACE("Registry::~Registry - end")
}

void Registry::run() 
{
	TRACE("Registry::run - start")
	
	sleep(1000); // wait startup completion

	while(true)
	{		
		try 
		{												
			wait();
			TRACE("Registry::run - start garbage collection")
			itsAction=Registry::GARBAGE_COLLECTION;
			forEach(); // Iterate all elements in queue
			TRACE("Registry::run - garbage collection done")
			release();	

			if(itsRunningFlag==false)
				break;			
				
			sleep(500); // wait 500 ms
		}
		catch(Exception& ex) 
		{
			release();
			TRACE(ex.getMessage().c_str())
		}
		catch(...)
		{
			release();
			TRACE("Registry::run : Unhandled exception")
		}
	}
	
	TRACE("Registry::run - end")		
}

// ++ v1.1
MQHANDLE Registry::findID()
{
	for(int cnt=1;cnt <= 0xFFFF; cnt++, itsNextHandleAvailable++)
	{
		if(itsNextHandleAvailable==0) // ++ v1.5
			itsNextHandleAvailable=1;
		
		if(at(itsNextHandleAvailable)==NULL)
		{
			MQHANDLE anHandle=itsNextHandleAvailable++;			
			return anHandle;
		} 		
	}	
	
	throw ThreadException("Registry::findID - no more handles available");
	return 0;
}

void Registry::add(MessageQueue* theQueue)
{
	TRACE("Registry::add - start")
	TRACE("Queue name=" << theQueue->getName())
	if(isShuttingDown())
	{
		TRACE("Registry::add: Action aborted on shutdown")
		return;
	}

	wait(); //++v1.4
	MQHANDLE aNewID=findID(); // ++ v1.1
	theQueue->setID(aNewID); // ++ v1.1
	set(aNewID,theQueue); // ++ v1.1
	push(theQueue);
	release(); //++v1.4 
	TRACE("Registry::add - end")
}

bool Registry::lookup(const char* theName,MQHANDLE& theID)
{
	TRACE("Registry::lookup - start")
	TRACE("Search=" << theName)
	if(isShuttingDown())
	{
		TRACE("Registry::lookup: Action aborted on shutdown")
		return false;
	}

	itsFoundID=0;
	itsAction=Registry::LOOKUP;
	itsFoundFlag=false;
	itsQueueToLookup=theName;
	wait();  //++v1.4
	forEach();
	release();  //++v1.4
	theID=itsFoundID;
	TRACE("Returned handle=" << theID)
	TRACE(((itsFoundFlag) ? "Found" : "Not found"))
	TRACE("Registry::lookup - end")
	return itsFoundFlag;
}

MessageQueue* Registry::lookup(MQHANDLE theID)
{
	TRACE("Registry::lookup - start")
	TRACE("Search=" << theID)
	if(isShuttingDown())
	{
		TRACE("Registry::lookup: Action aborted on shutdown")
		return NULL;
	}
	
	itsAction=Registry::LOOKUP1;
	itsFoundFlag=false;
	itsIDToFind=theID;	
	itsMessageQueue=NULL;

	wait();  //++v1.4
	forEach();
	release();  //++v1.4

	TRACE(((itsFoundFlag) ? "Found" : "Not found"))
	TRACE("Registry::lookup - end")
	return itsMessageQueue;
}

bool Registry::isStillAvailable(MQHANDLE theTarget)
{
	TRACE("Registry::isAvailable - start")
	TRACE("Handle=" << theTarget)
	if(isShuttingDown())
	{
		TRACE("Registry::isStillAvailable: Action aborted on shutdown")
		return false;
	}

	bool ret=false;
	wait();  //++v1.4
	MessageQueue* aQueue=(MessageQueue*)at(theTarget);
	release();  //++v1.4
	if(aQueue!=0)
	{
		TRACE("MessageQueue found with name=" << aQueue->getName())
		if(aQueue->isRunning())
		{
			TRACE("State=Running")
			ret=true;
		}
		else
		{
			TRACE("State=Not running")
		}	
	}		
	TRACE(((ret) ? "Available" : "Not available"))
	TRACE("Registry::isAvailable - end")
	return ret;
}

void Registry::remove(MessageQueue* theQueue)
{
	TRACE("Registry::remove - start")
	TRACE("Queue name=" << theQueue->getName())
	if(isShuttingDown())
	{
		TRACE("Registry::remove: Action aborted on shutdown")
		return;
	}

	itsAction=Registry::REMOVE;
	itsMessageQueue=theQueue;
	wait();  //++v1.4
	forEach();
	release();  //++v1.4
	TRACE("Registry::remove - end")
}

void Registry::post(MQHANDLE theTarget,Message* theMessage)
{
	TRACE("Registry::post - start")
	if(isShuttingDown())
	{
		TRACE("Registry::post: Action aborted on shutdown")
		return;
	}
	
	wait();  //++v1.4
	MessageQueue* aQueue=(MessageQueue*)at(theTarget);
	release();  //++v1.4
	if(aQueue!=0)
		aQueue->post(theMessage);
	TRACE("Registry::post - end")
}

void Registry::broadcast(Message* theMessage)
{
	TRACE("Registry::broadcast - start")
	if(isShuttingDown())
	{
		TRACE("Registry::broadcast: Action aborted on shutdown")
		return;
	}

	itsAction=Registry::BROADCAST;
	itsMessage=theMessage;
	wait();  //++v1.4
	forEach();
	release();  //++v1.4
	delete theMessage;
	TRACE("Registry::broadcast - end")
}

void Registry::dump()
{
	TRACE("Registry::dump - start")
	LOG("Start of registry dump:")
	itsAction=Registry::DUMP;
	wait();
	forEach();
	release();
	LOG("End of dump")
	TRACE("Registry::dump - end")
}

bool Registry::onIteration(LinkedElement* theElement)
{
	TRACE("Registry::onIteration - start")
	bool ret=true;
	MessageQueue* aQueue=(MessageQueue*)theElement->getObject();
	TRACE("Queue name=" << aQueue->getName())

	switch(itsAction)
	{
		case Registry::REMOVE:
			if(itsMessageQueue==aQueue)
			{
				unset(aQueue->getID());
				TRACE(aQueue->getName() << " removed from registry")
				theElement->remove();
				delete theElement;
				itsElementCount--;
				ret=false;
			}
			break;

		case Registry::BROADCAST:
			{
				Message* aMessage=itsMessage->clone();
				if(aMessage!=NULL && aQueue->getID()!=aMessage->getSender()) // v1.5
					aQueue->post(aMessage);
				TRACE("Message sent to" << aQueue->getName())
			}
			break;

		case Registry::LOOKUP:
			if(aQueue->is(itsQueueToLookup.c_str(),itsFoundID))
			{
				itsFoundFlag=true;
				ret=false;
				TRACE(aQueue->getName() << " found with ID=" << aQueue->getID())
			}
			break;

		case Registry::LOOKUP1:
			if(aQueue->getID()==itsIDToFind)
			{
				itsFoundFlag=true;
				ret=false;
				itsMessageQueue=aQueue;
				TRACE(aQueue->getName() << " found with ID=" << aQueue->getID())
			}
			break;


		case Registry::GARBAGE_COLLECTION:
			if(!aQueue->isRunning())
			{
				string msg=string("Thread ")+aQueue->getName()+string(" not running. Removed from registry.");
				WARNING(msg.c_str())
				TRACE(aQueue->getName() << " not running. Removed from registry")
				unset(aQueue->getID());
				theElement->remove();
				delete theElement;
				itsElementCount--;
			}
			break;

		case Registry::DUMP:
			LOG(aQueue->getName())
			break;
	}
	
	TRACE(((ret) ? "Continue" : "Break loop"))
	TRACE("Registry::onIteration - end")
	return ret;
}				

void Registry::deleteObject(void* theObject)  //++ v1.5
{
	delete (MessageQueue*)theObject; 
}


