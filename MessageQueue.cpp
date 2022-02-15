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

Registry* MessageQueue::itsRegistry=NULL;
Decoupler* Decoupler::itsDefaultDecoupler=NULL;	

bool Message::is(const char* theName)
{
	TRACE("Message::is - start")
	TRACE("Name=" << theName)
	TRACE("Current message class=" << getClass())
	bool ret=false;
	if(itsClassName==theName)
	{
		ret=true;
		TRACE("Match")
	}	
	TRACE("Message::is - end")
	return ret;
}

void MessageQueue::add(MessageQueue* theQueue)
{
	TRACE("MessageQueue::add(static) - start")
	if(itsRegistry==NULL)
		itsRegistry=new Registry("DefaultRegistry");	
	itsRegistry->add(theQueue);
	TRACE(theQueue->getName() << " added to registry with ID=" << theQueue->getID())	
	TRACE("MessageQueue::add(static) - end")
}

bool MessageQueue::lookup(const char* theName,MQHANDLE& theID)
{
	TRACE("MessageQueue::lookup(static) - start")
	bool ret=false;
	theID=-1;
	if(itsRegistry!=NULL)
		ret=itsRegistry->lookup(theName,theID);
	TRACE("Returned handle=" << theID)
	TRACE(((ret) ? "Found" : "Not found"))
	TRACE("MessageQueue::lookup(static) - end")
	return ret;
}

MessageQueue* MessageQueue::lookup(MQHANDLE theID)
{
	TRACE("MessageQueue::lookup(static) - start")
	MessageQueue* ret=NULL;
	if(itsRegistry!=NULL)
		ret=itsRegistry->lookup(theID);
	TRACE("MessageQueue::lookup(static) - end")
	return ret;
}

bool MessageQueue::isStillAvailable(MQHANDLE theTarget)
{
	TRACE("MessageQueue::isStillAvailable(static) - start")
	bool ret=false;
	if(itsRegistry!=NULL)
		ret=itsRegistry->isStillAvailable(theTarget);
	TRACE(((ret) ? "Available" : "Not available"))
	TRACE("MessageQueue::isStillAvailable(static) - end")
	return ret;
}

void MessageQueue::remove(MessageQueue* theQueue)
{
	TRACE("MessageQueue::remove(static) - start")
	TRACE("Target=" << theQueue->getName());
	if(itsRegistry!=NULL)
		itsRegistry->remove(theQueue);	
	TRACE("MessageQueue::remove(static) - end")
}

void MessageQueue::post(MQHANDLE theTarget,Message* theMessage)
{
	TRACE("MessageQueue::post(static) - start")
	TRACE("Target=" << theTarget);
	if(itsRegistry!=NULL)
		itsRegistry->post(theTarget,theMessage);	
	TRACE("MessageQueue::post(static) - end")
}

void MessageQueue::broadcast(Message* theMessage)
{
	TRACE("MessageQueue::broadcast(static) - start")
	if(itsRegistry!=NULL)
		itsRegistry->broadcast(theMessage);	
	TRACE("MessageQueue::broadcast(static) - end")
}
		
void MessageQueue::dump()
{
	TRACE("MessageQueue::dump(static) - start")
	if(itsRegistry!=NULL)
		itsRegistry->dump();	
	TRACE("MessageQueue::dump(static) - end")
}

void MessageQueue::waitForCompletion()
{ 
	TRACE("MessageQueue::waitForCompletion - start")
	if(itsRegistry!=NULL)
	{
		delete itsRegistry;
		itsRegistry=NULL;
	}
	TRACE("MessageQueue::waitForCompletion - end")
}

MessageQueue::MessageQueue(const char* theThreadName) : Thread(theThreadName)
{
	TRACE("MessageQueue constructor - start")
	TRACE("Thread name=" << theThreadName)
	start();
	add(this);
	TRACE("MessageQueue constructor - end")
}
	
MessageQueue::~MessageQueue() 
{
	TRACE("MessageQueue destructor - start")
	stop(false); // ++ v1.5
	remove(this);
	free();	
	TRACE("MessageQueue destructor - end")
}

bool MessageQueue::is(const char* theName,MQHANDLE& theID)
{
	TRACE("MessageQueue::is - start")
	TRACE("Name=" << theName)
	TRACE("Current queue=" << getName())
	bool ret=false;
	theID=-1;
	if(Thread::is(theName))
	{
		theID=itsID;
		ret=true;
	}	
	TRACE(((ret) ? "Found" : "Not found"))
	TRACE("MessageQueue::is - end")
	return ret;
}

void MessageQueue::flush()
{
	TRACE("MessageQueue::flush - start")
	wait();
	free();
	release();
	TRACE("MessageQueue::flush - end")
}

void MessageQueue::post(Message* theMessage)
{
	TRACE("MessageQueue::post - start")
	
	if(isShuttingDown())
	{
		delete theMessage;
		TRACE("MessageQueue::post: Action aborted on shutdown")
		return;
	}
		
	try 
	{
		wait();
		
		push(theMessage);

		if(isSuspended()==true)
		{
			resume();
		}

		release();
	}
	catch(Exception& ex) 
	{
		release();
		onException(ex);
	}
	
	TRACE("MessageQueue::post - end")		
}

void MessageQueue::shutdown()
{
	TRACE("MessageQueue::shutdown - start")		
	itsRunningFlag=false;
	if(isSuspended()==true)
		resume();
	TRACE("MessageQueue::shutdown - end")		
}

void MessageQueue::run() 
{
	TRACE("MessageQueue::run - start")
	TRACE("Thread name=" << getName())

	while(true)
	{
		TESTCANCEL					

		if(m_hThread!=0)
		{		
			try 
			{									
				while(true)
				{
					TESTCANCEL					
					wait();
					TESTCANCEL					

					if(isEmpty())
					{
						release();
						break;
					}
									
					TRACE("MessageQueue::run - dequeue a new message")
					TRACE("Thread name=" << getName())
					Message* aMessage=(Message*)pop();
					release();	
					
					TESTCANCEL
										
					if(!isShuttingDown())
						onMessage(aMessage);
						
					delete aMessage;
				}		
	
				TESTCANCEL							
				suspend();		
			}
			catch(Exception& ex) 
			{
				release();
				onException(ex);
			}
			catch(...)
			{
				release();
				DISPLAY("MessageQueue::run(" << getName() << ") : Unhandled exception")
			}			
		}	
	}
	
	TRACE("MessageQueue::run - end")		
}

void MessageQueue::onException(Exception& ex)
{
	DISPLAY("MessageQueue::run(" << getName() << ") : " << ex.getMessage().c_str())
}

void Decoupler::deferredPost(MQHANDLE theTarget,Message* theMessage)
{
	TRACE("Decoupler::deferredPost(static) - start")	
	if(itsDefaultDecoupler==NULL)
		itsDefaultDecoupler=new Decoupler("DefaultDecoupler");

	if(!Thread::isShuttingDown()) // Avoid use of itsDefultDecoupler during shutdown cleanup
		itsDefaultDecoupler->post(theTarget,theMessage);
	TRACE("Decoupler::deferredPost(static) - end")	
}

void Decoupler::deferredBroadcast(Message* theMessage)
{
	TRACE("Decoupler::deferredBroadcast(static) - start")	
	if(itsDefaultDecoupler==NULL)
		itsDefaultDecoupler=new Decoupler("DefaultDecoupler");

	if(!Thread::isShuttingDown()) // Avoid use of itsDefultDecoupler during shutdown cleanup
		itsDefaultDecoupler->post(0,theMessage);
	TRACE("Decoupler::deferredBroadcast(static) - end")	
}

void Decoupler::post(MQHANDLE theTarget,Message* theMessage)
{
	TRACE("Decoupler::post - start")	
	MessageQueue::post(new DeferredMessage(theTarget,theMessage));
	TRACE("Decoupler::post - end")	
}

void Decoupler::onMessage(Message* theMessage)
{
	TRACE("Decoupler::onMessage - start")	
	if(theMessage->is("DeferredMessage"))
	{
		DeferredMessage* aMessage=(DeferredMessage*)theMessage;
		if(aMessage->getTarget()==0)
			MessageQueue::broadcast(aMessage->getMessage());
		else	
			MessageQueue::post(aMessage->getTarget(),aMessage->getMessage());
	}
	TRACE("Decoupler::onMessage - end")	
}

