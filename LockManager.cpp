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
#include "LockManager.h"
#include "Trace.h"
#include "GeneralHashFunctions.h"

LockManagerClient::LockManagerClient(const char* theName, const char* theHost,int thePort, const char* theTarget) 
				   :Client(theName, theHost, thePort, theTarget)
{
	TRACE("LockManagerClient::LockManagerClient - start")

	TRACE("LockManagerClient::LockManagerClient - end")
}

LockManagerClient::LockManagerClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Encription* theEncr) 
				   :Client(theName, theHost, thePort, theTarget)
{
	TRACE("LockManagerClient::LockManagerClient - start")
	setEncription(theEncr);
	TRACE("LockManagerClient::LockManagerClient - end")
}

LockManagerClient::~LockManagerClient()
{
	TRACE("LockManagerClient::~LockManagerClient - start")

	TRACE("LockManagerClient::~LockManagerClient - end")
}

bool LockManagerClient::lock(const char* theResource)
{
	TRACE("LockManagerClient::lock - start")
	bool ret=false;	
	bool found=false;

	list<LockManagerSession>::iterator i;
	for (i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if((*i).resource.compare(theResource)==0)
		{
			found=true;
			break;	
		}			
	}

	if(!found)
	{
		itsTxStructure.free();	
		StringProperty* aLockTypeProperty=new StringProperty("LT");
		aLockTypeProperty->set("LCK");			
		itsTxStructure.add(aLockTypeProperty);
	
		StringProperty* aResourceProperty=new StringProperty("RN");
		aResourceProperty->set(theResource);		
		itsTxStructure.add(aResourceProperty);
		TRACE("Resource name=" << theResource)
	
		LockManagerSession aNewSession;
		aNewSession.time=Timer::time();
		aNewSession.token=0;
		aNewSession.resource=theResource;
		aNewSession.state=LM_WAIT_ACK;
		aNewSession.client=JSHash(theResource)+threadID()+Timer::time()+rand();
	
		LongIntProperty* aClientTokenProperty=new LongIntProperty("CT");
		aClientTokenProperty->set(aNewSession.client);		
		itsTxStructure.add(aClientTokenProperty);
		TRACE("Client token=" << aNewSession.client)
	
		itsSessions.push_back(aNewSession);
	
		string aString;
		encodeProperties(itsTxStructure,aString);
		sendMessage(aString);
		ret=true;
	}

	TRACE("LockManagerClient::lock - end")
	return ret;
}

bool LockManagerClient::unlock(const char* theResource)
{
	TRACE("LockManagerClient::unlock - start")
	bool ret=false;	
	bool found=false;

	itsTxStructure.free();	
	StringProperty* aLockTypeProperty=new StringProperty("LT");
	aLockTypeProperty->set("ULK");		
	itsTxStructure.add(aLockTypeProperty);

	list<LockManagerSession>::iterator i;
	for (i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if((*i).resource.compare(theResource)==0)
		{
			found=true;
			break;	
		}			
	}

	if(found)
	{	
		LongIntProperty* aTokenProperty=new LongIntProperty("TK");
		aTokenProperty->set((*i).token);		
		itsTxStructure.add(aTokenProperty);
		TRACE("Session token=" << (*i).token)

		LongIntProperty* aClientTokenProperty=new LongIntProperty("CT");
		aClientTokenProperty->set((*i).client);		
		itsTxStructure.add(aClientTokenProperty);
		TRACE("Client token=" << (*i).client)

		(*i).time=Timer::time();
		(*i).state=LM_WAIT_ACK;

		string aString;
		encodeProperties(itsTxStructure,aString);
		sendMessage(aString);
		ret=true;
	}

	TRACE("LockManagerClient::unlock - end")
	return ret;
}

void LockManagerClient::success(string theBuffer)
{
	TRACE("LockManagerClient::success - start")

	decodeProperties(theBuffer,itsResponse);

	LockManagerResult result=LM_VOID;

	Property* aProperty=itsResponse.get("RS");
	if(aProperty!=NULL && aProperty->is(PROPERTY_SHORTINT))
	{
		result=(LockManagerResult)((ShortIntProperty*)aProperty)->get();
		TRACE("Result=" << result)
	}

	unsigned long token=0;
	aProperty=itsResponse.get("TK");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		token=((LongIntProperty*)aProperty)->get();
		TRACE("Session token=" << token)
	}

	unsigned long client=0;
	aProperty=itsResponse.get("CT");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		client=((LongIntProperty*)aProperty)->get();
		TRACE("Client token=" << client)
	}

	bool found=false;
	list<LockManagerSession>::iterator i;
	for (i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if((*i).client==client)
		{
			found=true;
			break;	
		}			
	}

	if(found)
	{
		LockManagerSession& aSession=*i;

		switch(result)
		{
				case LM_NOT_ALLOWED:
					TRACE("Lock manager exception")
					aSession.state=LM_FAILED;
					aSession.token=0;
					onCompletion(aSession);
					itsSessions.erase(i);
					break;
				
				case LM_LOCKED:
					TRACE("Lock acquired")
					aSession.token=token;
					aSession.state=LM_STOP;
					onCompletion(aSession);
					break;
				
				case LM_UNLOCKED:
					TRACE("Unlocked")
					aSession.state=LM_STOP;
					aSession.token=0;
					onCompletion(aSession);
					itsSessions.erase(i);
					break;
								
				default:
					TRACE("Return value not allowed")
					aSession.state=LM_FAILED;
					aSession.token=0;
					onCompletion(aSession);			
					itsSessions.erase(i);
		}
	}
	else
	{
		TRACE("Client session not found. Token=" << token)	
	}
		
	TRACE("LockManagerClient::success - end")
}

void LockManagerClient::onWakeup(Wakeup* theMessage)
{
	TRACE("LockManagerClient::onWakeup - start")

	for(list<LockManagerSession>::iterator i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		LockManagerSession& aSession=*i;

		if(Timer::time()-aSession.time>LM_TIMEOUT)
		{
			TRACE("Session timeout. Token=" << aSession.token << " dTime=" << (theMessage->getTime()-aSession.time))
			aSession.state=LM_FAILED;
			aSession.token=0;
			onCompletion(aSession);			
			break;
		}			
	}

	Client::onWakeup(theMessage);
	
	TRACE("LockManagerClient::onWakeup - end")
}

LockManagerServer::LockManagerServer(const char* theName)
				   :Server(theName)
{
	TRACE("LockManagerServer::LockManagerServer - start")
	SCHEDULE(this,500);
	TRACE("LockManagerServer::LockManagerServer - end")
}

LockManagerServer::LockManagerServer(const char* theName,Encription* theEncr)
				   :Server(theName)
{
	TRACE("LockManagerServer::LockManagerServer - start")
	setEncription(theEncr);	
	SCHEDULE(this,500);
	TRACE("LockManagerServer::LockManagerServer - end")
}

LockManagerServer::~LockManagerServer()
{
	TRACE("LockManagerServer::~LockManagerServer - start")
	
	TRACE("LockManagerServer::~LockManagerServer - end")
}
	
string LockManagerServer::service(string theBuffer)
{
	TRACE("LockManagerServer::service - start")
	string ret;
	ListProperty aResponse;

	decodeProperties(theBuffer,itsRxStructure);

	Property* aProperty=itsRxStructure.get("CT");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		unsigned long aClientToken=((LongIntProperty*)aProperty)->get();

		aProperty=itsRxStructure.get("LT");
		if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
		{
			string btype=((StringProperty*)aProperty)->get();
			TRACE("Lock type=" << btype)
	
			if(btype.compare("LCK")==0)
			{			
				receiveLock(aClientToken,aResponse);
			}
			else if(btype.compare("ULK")==0)
			{
				receiveUnlock(aClientToken,aResponse);
			}
			else
			{
				setResponse(aResponse,LM_NOT_ALLOWED,aClientToken);
			}	
		}
	}

	encodeProperties(aResponse,ret);

	TRACE("LockManagerServer::service - end")
	return ret;
}

void LockManagerServer::setResponse(ListProperty& theResponse,LockManagerResult theCode,unsigned long theClientToken,unsigned long theToken)
{
	TRACE("LockManagerServer::setResponse - start")
	TRACE("Result=" << theCode)
	TRACE("Token=" << theToken)
	
	ShortIntProperty* aResultProperty=new ShortIntProperty("RS");
	aResultProperty->set(theCode);		
	theResponse.add(aResultProperty);
	
	if(theToken!=0L)
	{
		LongIntProperty* aTokenProperty=new LongIntProperty("TK");
		aTokenProperty->set(theToken);		
		theResponse.add(aTokenProperty);
	}

	LongIntProperty* aClientTokenProperty=new LongIntProperty("CT");
	aClientTokenProperty->set(theClientToken);		
	theResponse.add(aClientTokenProperty);
	TRACE("LockManagerServer::setResponse - end")
}

unsigned long LockManagerServer::lock(string theResourceName)
{
	TRACE("LockManagerServer::lock - start")
	wait();

	LockManagerSession aNewSession;
	aNewSession.time=Timer::time();
	aNewSession.token=RSHash(theResourceName)+Timer::time();
	aNewSession.resource=theResourceName;
	aNewSession.client=0;

	bool found=false;
	list<LockManagerSession>::iterator i;
	for (i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if((*i).resource.compare(theResourceName)==0)
		{
			found=true;
			break;	
		}			
	}

	if(!found)
	{	
		onLock(aNewSession);
		itsSessions.push_back(aNewSession);
		TRACE("Resource locked. Name=" << aNewSession.resource)
	}
	else
	{
		aNewSession.token=0;
		TRACE("Resource already locked. Name=" << aNewSession.resource)
	}

	release();
	
	TRACE("LockManagerServer::lock - end")
	return aNewSession.token;
}

void LockManagerServer::receiveLock(unsigned long theClientToken,ListProperty& theResponse)
{
	TRACE("LockManagerServer::receiveLock - start")
	TRACE("Client token=" << theClientToken)

	Property* aProperty=itsRxStructure.get("RN");
	if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
	{
		LockManagerSession aNewSession;
		aNewSession.time=Timer::time();
		
		string name=((StringProperty*)aProperty)->get();
		TRACE("Resource mame= " << name)
		aNewSession.token=RSHash(name)+rand()*rand();
		aNewSession.resource=name;
		aNewSession.client=theClientToken;

		bool found=false;
		list<LockManagerSession>::iterator i;
		for (i=itsSessions.begin(); i != itsSessions.end(); i++)
		{
			if((*i).resource.compare(name)==0)
			{
				found=true;
				break;	
			}			
		}

		if(!found)
		{	
			onLock(aNewSession);
			itsSessions.push_back(aNewSession);
			setResponse(theResponse,LM_LOCKED,theClientToken,aNewSession.token);
			TRACE("Resource locked. Name=" << aNewSession.resource)
		}
		else if(found && (*i).client==theClientToken)
		{
			setResponse(theResponse,LM_LOCKED,theClientToken,aNewSession.token);
			TRACE("Resource already locked. Name=" << aNewSession.resource)
		}
		else
		{
			setResponse(theResponse,LM_NOT_ALLOWED,theClientToken);
			TRACE("Resource already locked. Name=" << aNewSession.resource)
		}
	}
	else
	{
		setResponse(theResponse,LM_NOT_ALLOWED,theClientToken);
		TRACE("Wrong parameters from peer")
	}

	TRACE("LockManagerServer::receiveLock - end")
}

void LockManagerServer::unlock(unsigned long theToken)
{
	TRACE("LockManagerServer::unlock - start")
	wait();
	
	bool found=false;
	list<LockManagerSession>::iterator i;
	for (i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if((*i).token==theToken)
		{
			found=true;
			break;	
		}			
	}
	
	if(found)
	{
		TRACE("Session found")
		TRACE("Resource unlocked. Name=" << (*i).resource)
		onUnlock(*i);
		itsSessions.erase(i);
	}
	else
	{
		TRACE("Session not found")			
	}

	release();
	
	TRACE("LockManagerServer::unlock - end")
}

void LockManagerServer::receiveUnlock(unsigned long theClientToken,ListProperty& theResponse)
{
	TRACE("LockManagerServer::receiveUnlock - start")
	TRACE("Client token=" << theClientToken)
	
	Property* aProperty=itsRxStructure.get("TK");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		unsigned long token=((LongIntProperty*)aProperty)->get();
		TRACE("Session token=" << token)
		
		bool found=false;
		list<LockManagerSession>::iterator i;
		for (i=itsSessions.begin(); i != itsSessions.end(); i++)
		{
			if((*i).token==token && (*i).client==theClientToken)
			{
				found=true;
				break;	
			}			
		}
		
		if(found)
		{
			TRACE("Session found")
			TRACE("Resource unlocked. Name=" << (*i).resource)
			setResponse(theResponse,LM_UNLOCKED,theClientToken);
			onUnlock(*i);
			itsSessions.erase(i);
		}
		else
		{
			TRACE("Session not found")			
			setResponse(theResponse,LM_UNLOCKED,theClientToken);
		}
	}
	else
	{
		TRACE("Wrong parameters from peer")
		setResponse(theResponse,LM_NOT_ALLOWED,theClientToken);
	}

	TRACE("LockManagerServer::receiveUnlock - end")
}

void LockManagerServer::onWakeup(Wakeup* theMessage)
{
	TRACE("LockManagerServer::onWakeup - start")
	
	for(list<LockManagerSession>::iterator i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if(Timer::time()-(*i).time>LM_SERVERTIMEOUT)
		{
			TRACE("Session timeout. Token=" << (*i).token << " dTime=" << (theMessage->getTime()-(*i).time))
			onUnlock(*i);
			itsSessions.erase(i);
			break;
		}			
	}
	
	TRACE("LockManagerServer::onWakeup - end")
}	

void LockManagerServer::onLock(LockManagerSession& theSession)
{
//	DISPLAY("Resource '" << theSession.resource << "' locked. Client token=" << theSession.client << ". Server token=" << theSession.token)
}	

void LockManagerServer::onUnlock(LockManagerSession& theSession)	
{
//	DISPLAY("Resource '" << theSession.resource << "' unlocked. Client token=" << theSession.client << ". Server token=" << theSession.token)
}	

