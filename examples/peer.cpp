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

#include "Session.h"
#include "Logger.h"
#include "FileSystem.h"
#include <string>
using namespace std;
//#define SILENT

class MyClient : public Client
{
public:
	MyClient(const char* theName, const char* theHost,int thePort, const char* theTarget)
		:Client(theName,theHost,thePort,theTarget)	{/* TO-DO */};
	virtual ~MyClient() {/* TO-DO */};

protected:
	virtual void success(string theBuffer) {/* TO-DO */};
	virtual void fail(string theError) {/* TO-DO */};	
};

// Server1-ThreadA: background service that receives data from an external 
// com-based application (for example from 50 marketplaces, 10 or more dataframes per second), 
// stores them in database and in a flatfile (both for logging, reporting and reuse)
// and sends then this data to a second service. 

typedef struct _Record
{
	int Value1;
	string Value2;
	short Value3;
} Record;

class ThreadA : public Observer
{
protected:
	Directory* itsDir;
	File* itsFile;
	ListProperty itsStructure;
	MyClient* itsClient;
	
public:
	ThreadA(const char* theThreadName,const char* theFileName,const char* theHost,int thePort, const char* theTarget) 
		: Observer(theThreadName)
	{
		TRACE("ThreadA::ThreadA - start")		
		itsClient=new MyClient((string("Client(")+theThreadName+string(")")).c_str(),theHost,thePort,theTarget);
		itsDir=Directory::getCurrent();
		itsFile=itsDir->create(theFileName);
		itsFile->create();		
		subscribe("ControlTopic");
		Message* anEvent=new Message("ThreadAEvent");
		anEvent->setSender(getID());
		Decoupler::deferredPost(getID(),anEvent);
		TRACE("ThreadA::ThreadA - end")
	};

	virtual ~ThreadA()
	{
		TRACE("ThreadA::~ThreadA - start")	
		delete itsDir;		
		TRACE("ThreadA::~ThreadA - end")
	};

protected:
	virtual Record* receive()
	{
		TRACE("ThreadA::receive - start")	
		// Simulate a socket receive		
		Record* aRecord=new Record; 
		sleep(100);
		aRecord->Value1=10;
		aRecord->Value2="A string";
		aRecord->Value3=3;
		TRACE("ThreadA::receive - end")	
		return aRecord;		
	};

	virtual string serialize(Record* theRecord)
	{
		TRACE("ThreadA::serialize - start")	
		itsStructure.free();

		LongIntProperty* aLongIntProperty=new LongIntProperty("Value1");
		aLongIntProperty->set(theRecord->Value1);		
		itsStructure.add(aLongIntProperty);
		
		StringProperty* aStringProperty=new StringProperty("Value2");
		aStringProperty->set(theRecord->Value2);		
		itsStructure.add(aStringProperty);

		ShortIntProperty* aShortIntProperty=new ShortIntProperty("Value3");
		aShortIntProperty->set(theRecord->Value3);		
		itsStructure.add(aShortIntProperty);
		
		string aString;
		encodeProperties(itsStructure,aString);

		TRACE("ThreadA::serialize - end")	
		return aString;
	}

	virtual void onLocal(Message* theMessage)	
	{
		TRACE("ThreadA::onLocal - start")			

		if(theMessage->is("ThreadAEvent") && theMessage->getSender()==getID() && !isShuttingDown())
		{
			TRACE("Receive from ext application")
			Record* aRecord=receive();
			string aMsg=serialize(aRecord);
			TRACE("Append record in log file")
			fstream& aStream=itsFile->get();
			aStream.write(aMsg.c_str(),aMsg.size());
			TRACE("Send record to ThreadB")
			itsClient->sendMessage(aMsg);
			delete aRecord;				
			TRACE("Recall this function")
			Message* anEvent=new Message("ThreadAEvent");
			anEvent->setSender(getID());
			Decoupler::deferredPost(getID(),anEvent);
		}
		
		TRACE("ThreadA::onLocal - end")
	};	

	virtual void onBroadcast(NetworkMessage* theMessage)
	{
		TRACE("ThreadA::onBroadcast - start")		
		string aMessage=theMessage->toString();
		// Do something
		TRACE("ThreadA::onBroadcast - end")		
	};
};

// Server2-ThreadB: this service works with the data from tread A (some computations for each marketplace 
// in a seperate thread) and sends then the prepared dataframes to thread C.

typedef struct _Dataframe
{
	int Data1;
	int Data2;
	int Data3;
} Dataframe;

class ThreadB : public Server
{
protected:
	MyClient* itsClient;
	ListProperty itsStructure;

public:
	ThreadB(const char* theThreadName,const char* theHost,int thePort, const char* theTarget) 
		: Server(theThreadName)
	{
		TRACE("ThreadB::ThreadB - start")		
		itsClient=new MyClient((string("Client(")+theThreadName+string(")")).c_str(),theHost,thePort,theTarget);
		subscribe("ControlTopic");
		TRACE("ThreadB::ThreadB - end")
	};

	virtual ~ThreadB()
	{
		TRACE("ThreadB::~ThreadB - start")	

		TRACE("ThreadB::~ThreadB - end")
	};

protected:
	virtual Record* deserialize(string theBuffer)
	{
		TRACE("ThreadB::deserialize - start")		

		decodeProperties(theBuffer,itsStructure);

		Record* aRecord=new Record;

		TRACE("Retrieving data from structure")

		Property* aProperty=itsStructure.get("Value1");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			aRecord->Value1=((LongIntProperty*)aProperty)->get();

		aProperty=itsStructure.get("Value2");
		if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
			aRecord->Value2=((StringProperty*)aProperty)->get();

		aProperty=itsStructure.get("Value3");
		if(aProperty!=NULL && aProperty->is(PROPERTY_SHORTINT))
			aRecord->Value3=((ShortIntProperty*)aProperty)->get();

		TRACE("ThreadB::deserialize - end")		
		return aRecord;		
	};

	virtual Dataframe* compute(Record* theRecord)
	{
		TRACE("ThreadB::compute - start")	
		Dataframe* aDataframe=new Dataframe;
		aDataframe->Data1=1;
		aDataframe->Data1=2;
		aDataframe->Data1=3;
		return aDataframe;
		TRACE("ThreadB::compute - start")	
	};

	virtual string serialize(Dataframe* theDataframe)
	{
		TRACE("ThreadB::serialize - start")	
		itsStructure.free();

		LongIntProperty* aLongIntProperty=new LongIntProperty("Data1");
		aLongIntProperty->set(theDataframe->Data1);		
		itsStructure.add(aLongIntProperty);
		
		aLongIntProperty=new LongIntProperty("Data2");
		aLongIntProperty->set(theDataframe->Data2);		
		itsStructure.add(aLongIntProperty);

		aLongIntProperty=new LongIntProperty("Data3");
		aLongIntProperty->set(theDataframe->Data3);		
		itsStructure.add(aLongIntProperty);
		
		string aString;
		encodeProperties(itsStructure,aString);

		TRACE("ThreadB::serialize - end")	
		return aString;
	}

	virtual string service(string theBuffer)
	{
		TRACE("ThreadB::service - start")			
		Record* aRecord=deserialize(theBuffer);
		Dataframe* aDataframe=compute(aRecord);
		string aMsg=serialize(aDataframe);
		itsClient->sendMessage(aMsg);		
		delete aRecord;
		delete aDataframe;		
		TRACE("ThreadB::service - end")	
		return "OK";	
	};

	virtual void onBroadcast(NetworkMessage* theMessage)
	{
		TRACE("ThreadB::onBroadcast - start")		
		string aMessage=theMessage->toString();
		// Do something
		TRACE("ThreadB::onBroadcast - end")		
	};
};

// Server1-ThreadC: this service receives data from thread B and do someting 

class ThreadC : public Server
{
protected:
	ListProperty itsStructure;

public:
	ThreadC(const char* theThreadName) 
		: Server(theThreadName)
	{
		TRACE("ThreadC::ThreadC - start")		
		subscribe("ControlTopic");
		TRACE("ThreadC::ThreadC - end")
	};

	virtual ~ThreadC()
	{
		TRACE("ThreadC::~ThreadC - start")	

		TRACE("ThreadC::~ThreadC - end")
	};

protected:
	virtual Dataframe* deserialize(string theBuffer)
	{
		TRACE("ThreadC::deserialize - start")		

		decodeProperties(theBuffer,itsStructure);

		Dataframe* aDataframe=new Dataframe;

		TRACE("Retrieving data from structure")

		Property* aProperty=itsStructure.get("Data1");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			aDataframe->Data1=((LongIntProperty*)aProperty)->get();

		aProperty=itsStructure.get("Data2");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			aDataframe->Data2=((LongIntProperty*)aProperty)->get();

		aProperty=itsStructure.get("Data3");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			aDataframe->Data3=((LongIntProperty*)aProperty)->get();

		TRACE("ThreadC::deserialize - end")		
		return aDataframe;		
	};

	virtual void compute(Dataframe* theDataframe)
	{
		TRACE("ThreadC::compute - start")	
		// Do something
		TRACE("ThreadC::compute - end")	
	};

	virtual string service(string theBuffer)
	{
		TRACE("ThreadC::service - start")		
		Dataframe* aDataframe=deserialize(theBuffer);
		compute(aDataframe);
		delete aDataframe;		
		TRACE("ThreadC::service - end")
		return "OK";	
	};
	
	virtual void onBroadcast(NetworkMessage* theMessage)
	{
		TRACE("ThreadC::onBroadcast - start")		
		string aMessage=theMessage->toString();
		// Do something
		TRACE("ThreadC::onBroadcast - end")		
	};
};

// Server2-ThreadD: the mainservice coordinates the first three services

class ThreadD : public Observer
{
private:

public:
	ThreadD(const char* theName) : Observer(theName)
	{
		TRACE("ThreadD::ThreadD - start")		
		SCHEDULE(this,2000);
		TRACE("ThreadD::ThreadD - end")
	};
	
	virtual ~ThreadD()
	{
		TRACE("ThreadD::~ThreadD - start")	

		TRACE("ThreadD::~ThreadD - end")
	};	

protected:
	virtual void onWakeup(Wakeup* theMessage)
	{
		TRACE("ThreadD::onWakeup - start")		
		publish("ControlTopic","A command");
		TRACE("ThreadD::onWakeup - end")		
	};
};

class MyMessageProxyFactory : public MessageProxyFactory
{
protected:

public:
	MyMessageProxyFactory(const char* theFactoryName,int theSocket)
		:MessageProxyFactory(theFactoryName,theSocket) {};

	~MyMessageProxyFactory() {};

protected:

	virtual void onNewConnection(string theAddress,unsigned short thePort)
	{
		ThreadB* aThreadB=new ThreadB("MyThreadB",theAddress.c_str(),thePort,"MyThreadC");
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP peer.cpp")
	DISPLAY("This example shows how to build a peer to peer system")
		
	int servertype=0;
	int lport=0;
	char* host=NULL;
	int hport=0;
	
	if(argv < 3)
	{
		DISPLAY("Server1 usage: peer -1 hostip port")
		DISPLAY("Server2 usage: peer -2 port")
		return 0;	
	}
	else if(string(argc[1]).compare("-1")==0 && argv==4)
	{
		servertype=1;
		DISPLAY("Server2 host name=" << argc[2])
		host=argc[2];
		DISPLAY("Server2 host port=" << argc[3])
		hport=atoi(argc[3]);
	}	
	else if(string(argc[1]).compare("-2")==0 && argv==3)
	{
		servertype=2;
		DISPLAY("Server1 host name=" << argc[2])
		lport=atoi(argc[2]);
	}	
	else
	{
		DISPLAY("Server1 usage: peer -1 hostip port")
		DISPLAY("Server2 usage: peer -2 port")
		return 0;	
	}

	try
	{
		if(servertype==1)
		{
	    	DISPLAY("Starting server1 threads...")
			STARTLOGGER("server1.log")
			LOG("!!!!!!! peer.cpp - server1 !!!!!!!")
		
			ThreadA* aThreadA=new ThreadA("MyThreadA","records.log",host,hport,"MyThreadB") ;
			ThreadC* aThreadC=new ThreadC("MyThreadC"); 

			main_sleep(100);			
		}
		else if(servertype==2)
		{			
	    	DISPLAY("Starting server2 threads...")
			STARTLOGGER("server2.log")
			LOG("!!!!!!! peer.cpp - server2 !!!!!!!")

			MyMessageProxyFactory aFactory("MyFactory",lport);
			ThreadD* aThreadD=new ThreadD("MyThreadD");

			main_sleep(100);			
		}

	    DISPLAY("...stopping threads...")
	    Thread::shutdownInProgress();
	    Thread::sleep(100);
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
	return 0;
}
