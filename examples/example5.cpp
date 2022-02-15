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

#include "MessageProxy.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;

class MyClient : public Observer
{
private:
	MQHANDLE itsProxy;	
	MQHANDLE itsServer;
	unsigned itsCnt;
	char* itsHost; 
	const char* itsTarget;
	bool itsLookupOK;

public:
	MyClient(const char* theName, char* theHost, const char* theTarget) : Observer(theName)
	{
		itsCnt=0;
		itsHost=theHost;
		itsTarget=theTarget;
		itsLookupOK=false;
		setEncription(new Rijndael128()); // ++v1.3
		SCHEDULE(this,200);
	};
	
	virtual ~MyClient() {};

protected:
	virtual void onLookup(LookupReplyMessage* theMessage)
	{
		if(!theMessage->isFailed())
		{
			itsServer=theMessage->getHandle();
			itsProxy=theMessage->getSender();
			TRACE("Server=" << itsServer)
			TRACE("Proxy=" << itsProxy)
			itsLookupOK=true;	
			TRACE("Lookup ok!")
			LOG("Lookup ok!")
		}
		else
		{
			TRACE("Lookup failed!")
			LOG("Lookup failed!")
		}
	};

	virtual NetworkMessage* generateMessage()
	{
		ostrstream aStream;
		aStream << "MyClient(" << getName() << ") message n." << ++itsCnt << ends;
		char* aString=aStream.str();
		int aLen=strlen(aString)+1;
		NetworkMessage* aMessage=new NetworkMessage(aString,aLen);
		aMessage->setSender(getID());
		aMessage->setTarget(itsServer);
		aMessage->setUnsolicited();
		delete [] aString;
		return aMessage;				
	};

	virtual void onWakeup(Wakeup* theMessage)
	{
		if(itsLookupOK==true && isStillAvailable(itsProxy))
		{
			post(itsProxy,generateMessage());
			TRACE("Unsolicited message sent n." << itsCnt)
			LOG("Unsolicited message sent")
		}
		else
		{
			itsLookupOK=false;	
			TRACE("Try to lookup service")	
			LOG("Try to lookup service")	
			MessageProxyFactory::lookupAt(itsHost,9000,itsTarget,this);			
		}
	};
};

class MyServer : public Observer
{
public:
	MyServer(const char* theName) : Observer(theName) 
	{
		setEncription(new Rijndael128()); // ++v1.3
	};
	
	~MyServer() {};

protected:
	virtual void onUnsolicited(NetworkMessage* theMessage)
	{
		ostrstream aRxStream;
		theMessage->toStream(aRxStream);
		aRxStream << ends;
		char* aRxString=aRxStream.str();
		TRACE("MyServer(" << getName() << ") receive='" << aRxString << "'")
		LOG(aRxString)
		delete [] aRxString;
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example5.cpp")
	DISPLAY("This example shows how to send unsolicited network messages")	

	bool client=false;
	char* host="127.0.0.1";
	
	if(argv==2)
	{
		DISPLAY("Host name=" << argc[1])
		host=argc[1];	
		client=true;
	}
	else if(argv!=1)
	{
		DISPLAY("Usage: example5 [hostip]")
		return 0;	
	}

	try
	{
		if(client==true)
		{
	    	DISPLAY("Starting client threads...")
	    	STARTLOGGER("client.log")
			LOG("!!!!!!! example5.cpp - client !!!!!!!")	    	
			MyClient* aClientA=new MyClient("MyClientA",host,"MyServerA");
			main_sleep(20);			
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
	    	STARTLOGGER("server.log")
			LOG("!!!!!!! example5.cpp - server !!!!!!!")	    	
			MessageProxyFactory aFactory("MyFactory",9000);
			MyServer* aServerA=new MyServer("MyServerA");
			main_sleep(100);			
		}

	    DISPLAY("...stopping threads...")
	    Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(Exception& ex) 
	{
		TRACE(ex.getMessage().c_str())
	}
	catch(...)
	{
		TRACE("Unhandled exception")	
	}		

    DISPLAY("...done!")
	if(client)    
    	DISPLAY("See client.log for details")
    else	
    	DISPLAY("See server.log for details")
	return 0;
}

