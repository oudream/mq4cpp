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

#include "Logger.h"
#include "Timer.h"
#include <string>
#include <strstream>
using namespace std;

class MyMessage : public Message
{
protected:
	string itsBuffer;
	
public:
	MyMessage() : Message("MyMessage") {};	

	virtual ~MyMessage() 
	{
	};

	void set(char* theBuffer) 
	{ 
		itsBuffer=theBuffer; 
	};

	virtual void toStream(ostream& theStream) { theStream << itsBuffer; };
	virtual string toString() { return itsBuffer; };
};

class MyClient : protected MessageQueue
{
protected:
	unsigned itsCounter;
	MQHANDLE itsServerA;
	MQHANDLE itsServerB;

public:
	MyClient(const char* theName, MQHANDLE theServerA,MQHANDLE theServerB) : MessageQueue(theName)
	{
		SCHEDULE(this,100);
		itsCounter=0;
		itsServerA=theServerA;
		itsServerB=theServerB;		
	};
	
	virtual ~MyClient() {};

protected:
	virtual void onMessage(Message* theMessage)
	{
		if(theMessage->is("Wakeup"))
		{
			Wakeup* aWakeup=(Wakeup*)theMessage;
			
			ostrstream aStream;
			aStream << "MyClient wakeup at ";
			aStream << aWakeup->getTime() << ends;
			char* aString=aStream.str();
			TRACE(aString)
			LOG(aString)
			delete [] aString;
			
			ostrstream aStream1;
			aStream1 << "Message n.";
			aStream1 << ++itsCounter << ends;
			char* aString1=aStream1.str();
			
			MyMessage* aMessage=new MyMessage();
			aMessage->setSender(getID());
			aMessage->set(aString1);
			
			delete [] aString1;
			
			if((itsCounter%2) == 1) 
				post(itsServerA,aMessage);
			else
				post(itsServerB,aMessage);
		}
	};
};

class MyServerA : public MessageQueue
{
public:
	MyServerA(): MessageQueue("MyServerA") {};
	virtual ~MyServerA() {};

protected:
	virtual void onMessage(Message* theMessage)
	{
		if(theMessage->is("MyMessage"))
		{
			MyMessage* aMessage=(MyMessage*)theMessage;
			ostrstream aStream;
			aStream << "MyServerA receive <";
			aStream << aMessage->toString() << ">" << ends;
			char* aString=aStream.str();
			TRACE(aString)
			LOG(aString)
			delete [] aString;
		}
	};
};

class MyServerB : public MessageQueue
{
public:
	MyServerB(): MessageQueue("MyServerB") {};
	virtual ~MyServerB() {};

protected:
	virtual void onMessage(Message* theMessage)
	{
		if(theMessage->is("MyMessage"))
		{
			MyMessage* aMessage=(MyMessage*)theMessage;
			ostrstream aStream;
			aStream << "MyServerB receive <";
			aStream << aMessage->toString() << ">" << ends;
			char* aString=aStream.str();
			TRACE(aString)
			LOG(aString)
			delete [] aString;
		}
	};
};

int main() 
{
	DISPLAY("MQ4CPP example3.cpp")
	DISPLAY("This example shows how to send indirect messages unsing handles")	
	LOG("!!!!!!! example3.cpp !!!!!!!")

	try
	{
	    DISPLAY("Starting client and server threads...")
		MyServerA* aServerA=new MyServerA();
		MyServerB* aServerB=new MyServerB();
		MyClient* aClient1=new MyClient("Client1",aServerA->getID(),aServerB->getID());
		MyClient* aClient2=new MyClient("Client2",aServerA->getID(),aServerB->getID());
		MyClient* aClient3=new MyClient("Client3",aServerA->getID(),aServerB->getID());
		MyClient* aClient4=new MyClient("Client4",aServerA->getID(),aServerB->getID());
		MyClient* aClient5=new MyClient("Client5",aServerA->getID(),aServerB->getID());
		MyClient* aClient6=new MyClient("Client6",aServerA->getID(),aServerB->getID());
		MyClient* aClient7=new MyClient("Client7",aServerA->getID(),aServerB->getID());
		MyClient* aClient8=new MyClient("Client8",aServerA->getID(),aServerB->getID());
		MyClient* aClient9=new MyClient("Client9",aServerA->getID(),aServerB->getID());
		MyClient* aClient10=new MyClient("Client10",aServerA->getID(),aServerB->getID());
		DISPLAY("...wait 20 secs...")	
		Thread::sleep(20*1000);
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
    DISPLAY("See messages.log for details")
	return 0;
}
