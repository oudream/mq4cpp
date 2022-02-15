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
#include "Session.h"
#include "Trace.h"

ReplicationHost::ReplicationHost(const char* theName, const char* theHost,int thePort, const char* theTarget)
	            :Client(theName,theHost,thePort,theTarget)
{
	TRACE("ReplicationHost::ReplicationHost - start")	
	itsState=STOP;
	TRACE("ReplicationHost::ReplicationHost - end")	
}

ReplicationHost::~ReplicationHost()
{
	TRACE("ReplicationHost::~ReplicationHost - start")	

	TRACE("ReplicationHost::~ReplicationHost - end")	
}

bool ReplicationHost::send(string theBuffer)
{ 	
	TRACE("ReplicationHost::send - start")	
	bool ret=Client::send(theBuffer);
	if(ret)
		itsState=WAIT;
	TRACE("ReplicationHost::send - end")	
	return ret; 	
}

void ReplicationHost::success(string theBuffer)
{
	TRACE("ReplicationHost::success - start")	
	itsState=SUCCESS;
	TRACE("ReplicationHost::success - end")	
}

void ReplicationHost::fail(string theError)
{
	TRACE("ReplicationHost::fail - start")	
	itsState=FAIL;
	TRACE("ReplicationHost::fail - end")	
}

Session::Session(const char* theName,bool theAutoCommit) : Server(theName)
{
	TRACE("Session::Session - start")	
	itsModified=false;
	itsAutoCommit=theAutoCommit;
	TRACE("Session::Session - end")	
}

Session::~Session()
{
	TRACE("Session::~Session - start")	
	if(!isShuttingDown())
	{
		for(vector<ReplicationHost*>::iterator i = itsReplicaList.begin(); i < itsReplicaList.end(); ++i)
			delete *i;	
	}
	itsReplicaList.clear();		
	TRACE("Session::~Session - end")	
}

void Session::addReplicationHost(char* theHostName,int thePort)
{
	TRACE("Session::addReplicationHost - start")	
	ostrstream aNewName;
	aNewName << getName() << "(" << itsReplicaList.size() << ")" << ends;
	char* aString=aNewName.str();
	TRACE("Added " << aString)
	wait();
	itsReplicaList.push_back(new ReplicationHost(aString,theHostName,thePort,getName()));
	release();
	delete [] aString;
	TRACE("Session::addReplicationHost - end")	
}	

char Session::getChar(const char* thePropertyName)
{
	TRACE("Session::getChar - start")	
	TRACE("Property=" << thePropertyName)
	char ret=0;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if(aProperty!=NULL)
	{
		TRACE("Property found in ListProperty")
		if(aProperty->is(PROPERTY_CHAR))
			ret=((CharProperty*)aProperty)->get();		
	}
	
	release();
	TRACE("Value=" << (int)ret)
	TRACE("Session::getChar - end")	
	return ret;
}	

void Session::setChar(const char* thePropertyName,char theValue)
{
	TRACE("Session::setChar - start")	
	TRACE("Property=" << thePropertyName)
	TRACE("Value=" << (int)theValue)
	itsModified=true;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if (aProperty == NULL)
	{
		TRACE("Add new Property in ListProperty")
		CharProperty* aProperty=new CharProperty(thePropertyName);
		aProperty->set(theValue);
		itsPropertiesList.add(aProperty);				
	}
	else
	{
		TRACE("Property already stored in ListProperty")
		if(aProperty->is(PROPERTY_CHAR))
			((CharProperty*)aProperty)->set(theValue);		
	}
	
	if(itsAutoCommit)
		replication();

	release();
	TRACE("Session::setChar - end")	
}	

short Session::getShortInt(const char* thePropertyName)
{
	TRACE("Session::getShortInt - start")	
	TRACE("Property=" << thePropertyName)
	short ret;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if(aProperty!=NULL)
	{
		TRACE("Property found in ListProperty")
		if(aProperty->is(PROPERTY_SHORTINT))
			ret=((ShortIntProperty*)aProperty)->get();		
	}
	
	release();
	TRACE("Value=" << ret)
	TRACE("Session::getShortInt - end")	
	return ret;
}	

void Session::setShortInt(const char* thePropertyName,short theValue)
{
	TRACE("Session::setShortInt - start")	
	TRACE("Property=" << thePropertyName)
	itsModified=true;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if (aProperty == NULL)
	{
		TRACE("Add new Property in ListProperty")
		ShortIntProperty* aProperty=new ShortIntProperty(thePropertyName);
		aProperty->set(theValue);
		itsPropertiesList.add(aProperty);				
	}
	else
	{
		TRACE("Property already stored in ListProperty")
		if(aProperty->is(PROPERTY_SHORTINT))
			((ShortIntProperty*)aProperty)->set(theValue);		
	}

	if(itsAutoCommit)
		replication();

	release();
	TRACE("Session::setShortInt - end")	
}	

long Session::getLong(const char* thePropertyName)
{
	TRACE("Session::getLong - start")
	TRACE("Property=" << thePropertyName)
	long ret=0;
	wait();	
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if(aProperty!=NULL)
	{
		TRACE("Property found in ListProperty")
		if(aProperty->is(PROPERTY_LONGINT))
			ret=((LongIntProperty*)aProperty)->get();		
	}
	
	release();
	TRACE("Value=" << ret)
	TRACE("Session::getLong - end")	
	return ret;
}	

void Session::setLong(const char* thePropertyName,long theValue)
{
	TRACE("Session::setLong - start")
	TRACE("Property=" << thePropertyName)
	TRACE("Value=" << theValue)
	itsModified=true;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if (aProperty == NULL)
	{
		TRACE("Add new Property in ListProperty")
		LongIntProperty* aProperty=new LongIntProperty(thePropertyName);
		aProperty->set(theValue);
		itsPropertiesList.add(aProperty);				
	}
	else
	{
		TRACE("Property already stored in ListProperty")
		if(aProperty->is(PROPERTY_LONGINT))
			((LongIntProperty*)aProperty)->set(theValue);		
	}
	
	if(itsAutoCommit)
		replication();

	release();
	TRACE("Session::setLong - end")
}	

string Session::getString(const char* thePropertyName)
{
	TRACE("Session::getString - start")
	TRACE("Property=" << thePropertyName)
	string ret;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if(aProperty!=NULL)
	{
		TRACE("Property found in ListProperty")
		if(aProperty->is(PROPERTY_STRING))
			ret=((StringProperty*)aProperty)->get();		
	}
	
	release();
	TRACE("Value=" << ret)
	TRACE("Session::getString - end")
	return ret;
}	

void Session::setString(const char* thePropertyName,const char* theValue)
{
	TRACE("Session::setString - start")
	TRACE("Property=" << thePropertyName)
	TRACE("Value=" << theValue)
	itsModified=true;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if (aProperty == NULL)
	{
		TRACE("Add new Property in ListProperty")
		StringProperty* aProperty=new StringProperty(thePropertyName);
		aProperty->set(theValue);
		itsPropertiesList.add(aProperty);				
	}
	else
	{
		TRACE("Property already stored in ListProperty")
		if(aProperty->is(PROPERTY_STRING))
			((StringProperty*)aProperty)->set(theValue);		
	}

	if(itsAutoCommit)
		replication();

	release();
	TRACE("Session::setString - end")
}	

ListProperty* Session::getList(const char* thePropertyName) //++v1.3
{
	TRACE("Session::getList - start")
	TRACE("Property=" << thePropertyName)
	ListProperty* ret=NULL;
	wait();
	
	Property* aProperty=itsPropertiesList.get(thePropertyName);
	if(aProperty!=NULL)
	{
		TRACE("Property found in ListProperty")
		if(aProperty->is(PROPERTY_LIST))
			ret=(ListProperty*)aProperty;		
	}
	
	release();
	TRACE("Session::getList - end")
	return ret;
}	

void Session::setList(const char* thePropertyName,ListProperty* theValue)  //++v1.3
{
	TRACE("Session::setList - start")
	TRACE("Property=" << thePropertyName)
	itsModified=true;
	wait();
	
	itsPropertiesList.remove(thePropertyName);
	itsPropertiesList.add(theValue);				

	if(itsAutoCommit)
		replication();

	release();
	TRACE("Session::setList - end")
}	

void Session::commit()
{
	TRACE("Session::commit - start")
	wait();
	if(itsModified)
		replication();
	release();
	TRACE("Session::commit - end")
}

void Session::replication()
{
	TRACE("Session::replication - start")

	if(itsReplicaList.size()>0)
	{
		TRACE("Found " << itsReplicaList.size() << " replications host(s)")

		ostrstream aStream;
		itsPropertiesList.serialize(aStream);
		TRACE("Serialization done")
	
		string aString;
		int aLen=aStream.pcount();
		char* aBuffer=aStream.str();
		DUMP("Session::replication Tx buffer",aBuffer,aLen);	

		aString.assign(aBuffer,aLen);
		delete [] aBuffer;
	
		TRACE("Sending to all replication host")
		for(vector<ReplicationHost*>::iterator i = itsReplicaList.begin(); i < itsReplicaList.end(); ++i)
		{
			ReplicationHost* aClient=*i;		
			aClient->sendMessage(aString);	
		}
	}
	
	itsModified=false;		
	TRACE("Session::replication - end")
}

string Session::service(string theBuffer)
{
	TRACE("Session::service - start")
	DUMP("Session::service Rx buffer",(char*)theBuffer.data(),theBuffer.length());	
	istrstream aStream(theBuffer.data(),theBuffer.length());
	wait();
	itsPropertiesList.deserialize(aStream,true);
	release();
	TRACE("Session::service - end")
	return "";	
}	

bool Session::store(const char* theFileName)
{
	TRACE("Session::store - start")
	TRACE("File name=" << theFileName)
	bool ret=false;
	try
	{
		ofstream aStream(theFileName);
		wait();
		itsPropertiesList.serialize(aStream);
		release();
		aStream.close();	 	
		ret=true;	 	
	}
	catch(...)
	{
		release();
		TRACE("Exception during storing of file " << theFileName)
	}
	TRACE("Session::store - end")
	return ret;
}

bool Session::load(const char* theFileName)
{
	TRACE("Session::load - start")
	TRACE("File name=" << theFileName)
	bool ret=false;
	try
	{
		ifstream aStream(theFileName);
		if(!aStream==false)
		{
			wait();
			itsPropertiesList.free();
			TRACE("Repository freed")
			itsPropertiesList.deserialize(aStream,true);
			TRACE("Deserialization done")
			replication();
			TRACE("Replication done")
			release();
			aStream.close();
			ret=true;
		}
		else
		{
			TRACE("File not found")
		}	 	
	}
	catch(...)
	{
		release();
		TRACE("Exception during loading of file " << theFileName)	
	}
	TRACE("Session::load - end")
	return ret;
}

StatefulServer::StatefulServer(const char* theName) : Server(theName)
{
	TRACE("StatefulServer::StatefulServer - start")
	ostrstream aStream;
	aStream << "Session(" << getName() << ")" << ends;
	char* aString=aStream.str();
	itsSession=new Session(aString);
	delete [] aString;
	TRACE("StatefulServer::StatefulServer - end")
}

StatefulServer::~StatefulServer() 
{ 
	TRACE("StatefulServer::~StatefulServer - start")
	if(!isShuttingDown())
		delete itsSession; 
	TRACE("StatefulServer::~StatefulServer - end")
}
	
void StatefulServer::addReplicationHost(char* theHostName,int thePort)
{ 
	TRACE("StatefulServer::addReplicationHost - start")
	itsSession->addReplicationHost(theHostName,thePort);
	TRACE("StatefulServer::addReplicationHost - end")
}	

