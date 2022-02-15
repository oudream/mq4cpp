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

#ifndef __MEMORYCHANNEL__
#define __MEMORYCHANNEL__

#include "Session.h"
#include "FileSystem.h"
#include "Exception.h"
#include <vector>
using namespace std;

#define MC_BLOCKSIZE 512
#define MC_TIMEOUT 10

typedef enum _MC_State 
{
	MC_STOP,
	MC_SENDING_HEADER,
	MC_SENDING_BLOCKS,
	MC_WAIT_LAST_ACK,
	MC_FAILED
} MemoryChannelState;

typedef enum _MC_Result
{
	MC_VOID,
	MC_NOT_ALLOWED,
	MC_SESSION_STARTED,
	MC_SESSION_FINISHED,
	MC_SESSION_CONTINUE
} MemoryChannelResult;

class MemoryChannelException : public Exception
{
private:
	string msg;
public:
	MemoryChannelException();
	MemoryChannelException(string m) { msg=m; };
	MemoryChannelException(const char* m) { msg=m; };
	virtual ~MemoryChannelException() {};
	virtual string getMessage() const { return msg; };
};	

typedef enum _MC_BlockState
{
	MC_UNCHANGED,
	MC_CHANGED,
	MC_TRANSFERING,
	MC_TRANSFERED
} MemoryBlockState;

class MemoryChannelClient : public Client
{
protected:
	MemoryChannelState itsState;

	char* itsBuffer;
	unsigned long itsSize;
	MemoryBlockState* itsMap;
	unsigned long itsBlocks;

	char* itsTxBuffer;
	unsigned long itsTxSize;
	MemoryBlockState* itsTxMap;
	unsigned long itsTxBlocks;

	unsigned long itsLastTx;
	unsigned long itsTxCnt;
	unsigned long itsSessionToken;
	unsigned long itsBlocksToTx;
	unsigned long itsBlocksTransmitted;

	ListProperty itsTxStructure;
	ListProperty itsResponse;	

public:
	MemoryChannelClient(const char* theName, const char* theHost,int thePort, const char* theTarget);
	virtual ~MemoryChannelClient();

protected:
	virtual void setSize(unsigned long theSize);
	virtual void set(char* theBuffer,unsigned long theSize);	
	virtual void set(unsigned long theDisplacement,long long theValue);
	virtual void set(unsigned long theDisplacement,long theValue);
	virtual void set(unsigned long theDisplacement,short theValue);
	virtual void set(unsigned long theDisplacement,char theValue);
	virtual void set(unsigned long theDisplacement,char* theSourcePtr,unsigned long theSize);
	virtual void set(unsigned long theDisplacement,unsigned long theSize,char theValue);
	virtual void commit(bool theDeltaTransfer=true);
	virtual void rollback();
	virtual bool fail() { return (itsState==MC_FAILED); }; 	
	virtual bool done() { return (itsState==MC_STOP || itsState==MC_FAILED); }; 	
		
	virtual void success(string theBuffer);
	virtual void fail(string theError);	
	virtual void sendHeader();
	virtual void sendBlock();
	virtual void onCompletion()=0; // To ovveride	
	virtual bool searchFirstBlock();
	virtual bool searchNextBlock();
	virtual char* blockAt(unsigned long theBlock);
	virtual void markBlocks(unsigned long theDisplacement,unsigned long theValue);
	virtual void onWakeup(Wakeup* theMessage);		
};

typedef struct _MC_Session
{
	unsigned long blocksReceived;
	unsigned long blocksToRx;
	unsigned long size;
	unsigned long token;
	unsigned long time;
	char* buffer;
	unsigned long blocks;
	unsigned long lastblock;
} MemoryChannelSession;

class MemoryChannelServer : public Server
{
protected:
	MemoryChannelSession itsSession;

	char* itsBuffer;
	unsigned long itsSize;

	ListProperty itsRxStructure;	
	
public:
	MemoryChannelServer(const char* theName);
	virtual ~MemoryChannelServer();

protected:
	virtual unsigned long getSize() { return itsSize; };
	virtual long long get64(unsigned long theDisplacement);
	virtual long get32(unsigned long theDisplacement);
	virtual short get16(unsigned long theDisplacement);
	virtual char get8(unsigned long theDisplacement);
	virtual void get(unsigned long theDisplacement,char* theDestPtr,unsigned long theSize);
	
	virtual string service(string theBuffer);
	virtual void receiveHeader(ListProperty& theResponse);
	virtual void receiveBlock(ListProperty& theResponse);	
	virtual void setResponse(ListProperty& theResponse,MemoryChannelResult theCode,unsigned long theToken=0);	
	virtual void onCompletion()=0; // To ovveride	
	virtual void onWakeup(Wakeup* theMessage);	
	virtual void copyBuffer();		
}; 


#endif


