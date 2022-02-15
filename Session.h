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

#ifndef __SESSION__
#define __SESSION__

#include "RequestReply.h"
#include "Properties.h"

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

class ReplicationHost : public Client
{
public:
	typedef enum { STOP, WAIT, SUCCESS, FAIL } ClientState; 	

protected:
	ClientState itsState;

public:
	ReplicationHost(const char* theName, const char* theHost,int thePort, const char* theTarget);
	virtual ~ReplicationHost();
	ClientState state() { return itsState; };
	virtual bool send(string theBuffer); 	
	
protected:
	virtual void success(string theBuffer);
	virtual void fail(string theError);	
};

class Session : protected Server
{
protected:
	bool itsModified;
	bool itsAutoCommit;
	ListProperty itsPropertiesList;
	vector<ReplicationHost*> itsReplicaList;

public:
	Session(const char* theName,bool theAutoCommit=false);
	virtual ~Session();
	void addReplicationHost(char* theHostName,int thePort);	
	char getChar(const char* thePropertyName);
	void setChar(const char* thePropertyName,char theValue);
	short getShortInt(const char* thePropertyName);
	void setShortInt(const char* thePropertyName,short theValue);
	long getLong(const char* thePropertyName);
	void setLong(const char* thePropertyName,long theValue);
	string getString(const char* thePropertyName);
	void setString(const char* thePropertyName,const char* theValue);
	void commit();
	bool store(const char* theFileName); //  ++ v1.5
	bool load(const char* theFileName); //  ++ v1.5
	
protected:
	virtual void replication();
	virtual string service(string theBuffer);
	ListProperty* getList(const char* thePropertyName); // v1.3
	void setList(const char* thePropertyName,ListProperty* theValue); // v1.3
}; 

class StatefulServer : public Server
{
protected:
	Session* itsSession;	
	
public:
	StatefulServer(const char* theName); 
	virtual ~StatefulServer();
	void addReplicationHost(char* theHostName,int thePort); 
};

#endif
