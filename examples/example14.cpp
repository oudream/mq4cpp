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
#include "Router.h"
#include <string>
#include <strstream>
using namespace std;

class MyClient : public Client
{
protected:
	bool breakSignal;
	bool ackSignal;
	unsigned long itsCnt;	
	
public:
	MyClient(const char* theName, const char* theTarget) 
		: Client(theName,theTarget)
	{
		itsCnt=0;
		breakSignal=false;
		ackSignal=false;
		send(generate());		
	};
	
	MyClient(const char* theName, char* theHost,int thePort, const char* theTarget) 
		: Client(theName,theHost,thePort,theTarget)
	{
		itsCnt=0;
		breakSignal=false;
		ackSignal=false;
		send(generate());		
	};
	
	virtual ~MyClient() {};

	virtual void signal() 
	{ 
		breakSignal=true; 
	
		while(!ackSignal)
		{
			Thread::sleep(100);
		};
		
		shutdown();
	};

protected:
	virtual string generate()
	{
		char buffer[256];
		ostrstream aStream(buffer,sizeof(buffer));
		aStream << "MyClient(" << getName() << ") message n." << ++itsCnt << ends;
		TRACE("Send: " << buffer)		
		return buffer;				
	};
	
	void success(string theBuffer)
	{
		TRACE("Message successfully sent. Received='" << theBuffer.c_str() << "'")
		if(!breakSignal)
			send(generate());
		else
			ackSignal=true;
		TRACE("Sending another.")
	};
	
	void fail(string theError)
	{
		LOG("MyClient - Service failed")		
		if(breakSignal)
			ackSignal=true;
	};
};

class MyServer : public Server
{
public:
	MyServer(const char* theName) : Server(theName) {};	
	
	virtual ~MyServer() {};

protected:
	string service(string theBuffer)
	{
		ostrstream aStream;
		aStream << "MyServer(" << getName() << ") receive='" << theBuffer.c_str() << "'" << ends; 
		char* aString=aStream.str();
		TRACE(aString)
		LOG(aString)
		delete [] aString;
		return "Message received";
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example14.cpp")
	DISPLAY("This example shows how to route request/reply messages")	
	
	try
	{
		DISPLAY("Starting threads...")
		LOG("!!!!!!! example14.cpp !!!!!!!")

		string anID=MessageProxyFactory::getUniqueNetID();
		MyClient* aClient=NULL;

		LocalhostRouter* aRouter=new LocalhostRouter();
		Switch* aSwitch1=new Switch("MySwitch1");
		MyServer* aServerA=new MyServer("MyServerA");
		MQHANDLE anHandleA=aSwitch1->addRouting("MyServerA");
		MyServer* aServerB=new MyServer("MyServerB");
		MQHANDLE anHandleB=aSwitch1->addRouting(aServerB);

		Switch* aSwitch2=new Switch("MySwitch2");
		MQHANDLE anHandleS=aSwitch1->addRouting(LOCALHOST,LOCALPORT,"MySwitch2");
		MyServer* aServerC=new MyServer("MyServerC");
		Thread::sleep(10); // To avoid deadlock on Switch2
		MQHANDLE anHandleC=aSwitch2->addRouting("MyServerC");
		MyServer* aServerD=new MyServer("MyServerD");
		MQHANDLE anHandleD=aSwitch2->addRouting(aServerD);
		aSwitch2->addRouting("MyTopic1",anHandleC);
		aSwitch2->addRouting("MyTopic2",anHandleD);

		DISPLAY("Request/Reply switched to MyServerA")
		LOG("----------------------------- TEST 1 --------------------------")
		aSwitch1->activate(anHandleA);
		aClient=new MyClient("MyClientL","MySwitch1");
		aClient->setTopic(anID.c_str());
		main_sleep(2);			
		aSwitch1->activate(anHandleB);
		DISPLAY("Request/Reply switched to MyServerB")
		LOG("----------------------------- TEST 2 --------------------------")
		main_sleep(2);			
		aSwitch1->activate(anHandleS,"MyTopic1");
		DISPLAY("Request/Reply switched twice to MyServerC")
		LOG("----------------------------- TEST 3 --------------------------")
		main_sleep(2);			
		aSwitch1->activate(anHandleS,"MyTopic2");
		DISPLAY("Request/Reply switched twice to MyServerD")
		LOG("----------------------------- TEST 4 --------------------------")
		main_sleep(2);			
		aClient->signal();
		DISPLAY("Request/Reply via LocalhostRouter to MyServerA")
		LOG("----------------------------- TEST 5 --------------------------")
		aClient=new MyClient("MyClientR1",LOCALHOST,LOCALPORT,"MyServerA");
		main_sleep(2);			
		aClient->signal();
		DISPLAY("Request/Reply via LocalhostRouter to MyServerB")
		LOG("----------------------------- TEST 6 --------------------------")
		aClient=new MyClient("MyClientR2",LOCALHOST,LOCALPORT,"MyServerB");
		main_sleep(2);			
		aClient->signal();

		DISPLAY("Remove routing from Switch1")
		aSwitch1->removeRouting(anHandleA);
		aSwitch1->removeRouting(anHandleB);
		DISPLAY("Remove routing from Switch2")
		aSwitch2->resetRouting();
		DISPLAY("...stopping threads...")
		Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(Exception& ex) 
	{
		DISPLAY(ex.getMessage().c_str())
	}
	catch(...)
	{
		DISPLAY("Unhandled exception")	
	}		

	DISPLAY("...done!")
	DISPLAY("See messages.log for details")
	return 0;
}
