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
#include "MemoryChannel.h"
#include "Trace.h"
#include "GeneralHashFunctions.h"
#include "Logger.h"

static char* MEMORY_OVERFLOW="MemoryChannel Exception: displacement overflows memory buffer";
static char* TRANSFER_IN_PROGRESS="MemoryChannel Exception: memory transfer still in progress";

MemoryChannelClient::MemoryChannelClient(const char* theName, const char* theHost,int thePort, const char* theTarget)
				    :Client(theName, theHost, thePort, theTarget)
{
	TRACE("MemoryChannelClient::MemoryChannelClient - start")
	itsState=MC_STOP;
	itsBuffer=NULL;
	itsSize=0;
	itsMap=NULL;
	itsBlocks=0;
	itsTxBuffer=NULL;
	itsTxSize=0;
	itsTxMap=NULL;
	itsTxBlocks=0;
	itsLastTx=0;
	itsTxCnt=0;
	itsSessionToken=0;
	itsBlocksToTx=0;
	itsBlocksTransmitted=0;
	TRACE("MemoryChannelClient::MemoryChannelClient - end")
}

MemoryChannelClient::~MemoryChannelClient()
{
	TRACE("MemoryChannelClient::~MemoryChannelClient - start")
	if(itsBuffer!=NULL) delete [] itsBuffer;
	if(itsMap!=NULL) delete [] itsMap;
	if(itsTxBuffer!=NULL) delete [] itsTxBuffer;
	if(itsTxMap!=NULL) delete [] itsTxMap;
	TRACE("MemoryChannelClient::~MemoryChannelClient - end")
}

void MemoryChannelClient::setSize(unsigned long theSize)
{
	TRACE("MemoryChannelClient::setSize - start")
	if(theSize!=0 && theSize!=itsSize)
	{
		TRACE("Resizing working buffer")
		if(itsBuffer!=NULL) delete [] itsBuffer;
		if(itsMap!=NULL) delete [] itsMap;
	
		TRACE("Allocating new buffer. Size=" << theSize)
		itsSize=theSize;
		itsBlocks=theSize/MC_BLOCKSIZE;
		itsBlocks+=(theSize%MC_BLOCKSIZE>0) ? 1 : 0;
		TRACE("Blocks=" << itsBlocks)
		itsBuffer=new char[itsBlocks*MC_BLOCKSIZE];
		itsMap=new MemoryBlockState[itsBlocks];
		
		TRACE("Initializing memory")	
		memset(itsBuffer,0,itsSize);	
		memset(itsMap,0,itsBlocks);			
	}
	TRACE("MemoryChannelClient::setSize - end")
}

void MemoryChannelClient::set(char* theBuffer,unsigned long theSize)
{
	TRACE("MemoryChannelClient::set - start")
	if(itsBuffer!=NULL) delete [] itsBuffer;
	if(itsMap!=NULL) delete [] itsMap;

	itsBuffer=theBuffer;	
	itsSize=theSize;
	itsBlocks=theSize/MC_BLOCKSIZE;
	itsBlocks+=(theSize%MC_BLOCKSIZE>0) ? 1 : 0;
	TRACE("Blocks=" << itsBlocks)
	itsMap=new MemoryBlockState[itsBlocks];		
	memset(itsMap,0,itsBlocks);			
	TRACE("MemoryChannelClient::set - end")
}

void MemoryChannelClient::commit(bool theDeltaTransfer)
{
	TRACE("MemoryChannelClient::commit - start")
	if(done())
	{
		if(itsTxBlocks!=itsBlocks)
		{
			TRACE("Resizing transmition map")
			delete [] itsTxMap;
			itsTxBlocks=itsBlocks;
			itsTxMap=new MemoryBlockState[itsTxBlocks];	
		}

		if(itsTxSize!=itsSize)
		{
			TRACE("Resizing transmition buffer")
			delete [] itsTxBuffer;
			itsTxSize=itsSize;
			itsTxBuffer=new char[itsBlocks*MC_BLOCKSIZE];	
		}
		
		TRACE("Copying buffer and map")
		memcpy(itsTxBuffer,itsBuffer,itsTxSize);
		
		if(theDeltaTransfer && itsMap!=NULL)	
			memcpy(itsTxMap,itsMap,itsTxBlocks*sizeof(MemoryBlockState));
		else
			for(unsigned int cnt=0; cnt < itsTxBlocks; cnt++)
				itsTxMap[cnt]=MC_CHANGED;
	
		memset(itsMap,0,itsBlocks);			
		sendHeader();
	}		
	else
	{
		TRACE("Commit failed because MemoryChannelClient is still transfering previous buffer")
		throw MemoryChannelException(TRANSFER_IN_PROGRESS);
	}		
	TRACE("MemoryChannelClient::commit - end")
}

void MemoryChannelClient::rollback()
{
	TRACE("MemoryChannelClient::rollback - start")
	if(itsSize!=itsTxSize)
	{
		TRACE("Resizing transmition buffer")
		delete [] itsBuffer;
		itsSize=itsTxSize;
		itsBuffer=new char[itsSize];	
	}
	
	if(itsBlocks!=itsTxBlocks)
	{
		TRACE("Resizing transmition map")
		delete [] itsMap;
		itsBlocks=itsTxBlocks;
		itsMap=new MemoryBlockState[itsBlocks];	
	}
	
	TRACE("Copying buffer and map")
	if(itsSize!=0)
		memcpy(itsBuffer,itsTxBuffer,itsSize);	
	
	if(itsBlocks!=0)
		memcpy(itsMap,itsTxMap,itsBlocks);	

	TRACE("MemoryChannelClient::rollback - end")
}

bool MemoryChannelClient::searchFirstBlock()
{
	TRACE("MemoryChannelClient::searchFirstBlock - start")
	
	itsBlocksTransmitted=0;
	itsBlocksToTx=0;
	itsTxCnt=0;
	bool found=false;
	for(unsigned long cnt=0;cnt < itsTxBlocks;cnt++)
	{
		if(itsTxMap[cnt]==MC_CHANGED)
		{
			if(!found)
			{
				TRACE("Found block to tx at="  << cnt)
				itsTxCnt=cnt;
				itsTxMap[cnt]=MC_TRANSFERING;
				found=true;
			}		

			itsBlocksToTx++;
		}
	}	
	
	TRACE("Blocks to tx found=" << itsBlocksToTx)
	TRACE("MemoryChannelClient::searchFirstBlock - end")
	return found; 
}

bool MemoryChannelClient::searchNextBlock()
{
	TRACE("MemoryChannelClient::searchNextBlock - start")

	bool found=false;
	itsTxMap[itsTxCnt]=MC_TRANSFERED;
	
	for(unsigned long cnt=itsTxCnt+1;cnt < itsTxBlocks;cnt++)
	{
		if(itsTxMap[cnt]==MC_CHANGED)
		{
			TRACE("Found block to tx at="  << cnt)
			itsTxCnt=cnt;
			itsTxMap[cnt]=MC_TRANSFERING;
			found=true;
			break;
		}		
	}	
	
	TRACE("MemoryChannelClient::searchNextBlock - end")
	return found; 
}

char* MemoryChannelClient::blockAt(unsigned long theBlock)
{
	if(theBlock>itsTxBlocks) 
		throw MemoryChannelException(MEMORY_OVERFLOW);
	return (itsTxBuffer+theBlock*MC_BLOCKSIZE); 
}

void MemoryChannelClient::sendHeader()
{
	TRACE("MemoryChannelClient::sendHeader - start")
	if(searchFirstBlock())
	{
		itsState=MC_SENDING_HEADER;
		itsTxStructure.free();
		
		if(itsBlocksToTx == 1)
			itsState=MC_WAIT_LAST_ACK;
		
		StringProperty* aBlockTypeProperty=new StringProperty("BT");
		aBlockTypeProperty->set("HDR");		
		itsTxStructure.add(aBlockTypeProperty);
		
		LongIntProperty* aTotBlocksProperty=new LongIntProperty("TB");
		aTotBlocksProperty->set(itsBlocksToTx);		
		itsTxStructure.add(aTotBlocksProperty);
		TRACE("Blocks to tx=" << itsBlocksToTx)
	
		LongIntProperty* aBlockIndexProperty=new LongIntProperty("BI");
		aBlockIndexProperty->set(itsTxCnt);		
		itsTxStructure.add(aBlockIndexProperty);
		TRACE("Block index=" << itsTxCnt)
	
		LongIntProperty* aBufferSizeProperty=new LongIntProperty("BS");
		aBufferSizeProperty->set(itsTxSize);		
		itsTxStructure.add(aBufferSizeProperty);
		TRACE("Buffer size=" << itsTxSize)
	
		StringProperty* aBufferProperty=new StringProperty("BF");
		aBufferProperty->set(blockAt(itsTxCnt),MC_BLOCKSIZE);		
		itsTxStructure.add(aBufferProperty);
	
		string aString;
		encodeProperties(itsTxStructure,aString);
		sendMessage(aString);
		itsLastTx=Timer::time();
		itsBlocksTransmitted++;
	}
	
	TRACE("MemoryChannelClient::sendHeader - end")
}

void MemoryChannelClient::sendBlock()
{
	TRACE("MemoryChannelClient::sendBlock - start")

	itsState=MC_SENDING_BLOCKS;
	itsTxStructure.free();

	if(searchNextBlock())
	{
		if(itsBlocksTransmitted+1>=itsBlocksToTx)
			itsState=MC_WAIT_LAST_ACK;
		
		LongIntProperty* aTokenProperty=new LongIntProperty("TK");
		aTokenProperty->set(itsSessionToken);		
		itsTxStructure.add(aTokenProperty);
		TRACE("Session token=" << itsSessionToken)
	
		StringProperty* aBlockTypeProperty=new StringProperty("BT");
		aBlockTypeProperty->set("BLK");		
		itsTxStructure.add(aBlockTypeProperty);
		
		LongIntProperty* aBlockIndexProperty=new LongIntProperty("BI");
		aBlockIndexProperty->set(itsTxCnt);		
		itsTxStructure.add(aBlockIndexProperty);
		TRACE("Block index=" << itsTxCnt)
	
		StringProperty* aBufferProperty=new StringProperty("BF");
		aBufferProperty->set(blockAt(itsTxCnt),MC_BLOCKSIZE);		
		itsTxStructure.add(aBufferProperty);
	
		string aString;
		encodeProperties(itsTxStructure,aString);
		sendMessage(aString);
		itsLastTx=Timer::time();
		itsBlocksTransmitted++;
	}
	else
	{
		TRACE("No more blocks to tx")
		throw MemoryChannelException("MemoryChannelClient::sendBlock: no more blocks to tx");
	}

	TRACE("MemoryChannelClient::sendBlock - end")
}

void MemoryChannelClient::success(string theBuffer)
{
	TRACE("MemoryChannelClient::success - start")

	decodeProperties(theBuffer,itsResponse);

	MemoryChannelResult result=MC_VOID;

	Property* aProperty=itsResponse.get("RS");
	if(aProperty!=NULL && aProperty->is(PROPERTY_SHORTINT))
	{
		result=(MemoryChannelResult)((ShortIntProperty*)aProperty)->get();
		TRACE("Result=" << result)
	}

	aProperty=itsResponse.get("TK");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		itsSessionToken=((LongIntProperty*)aProperty)->get();
		TRACE("Session token=" << itsSessionToken)
	}

	switch(result)
	{
			case MC_NOT_ALLOWED:
				TRACE("Memory transfer exception")
				itsState=MC_FAILED;
				onCompletion();
				break;
			
			case MC_SESSION_STARTED:
			case MC_SESSION_FINISHED:
			case MC_SESSION_CONTINUE:		
				switch(itsState)
				{
					case MC_WAIT_LAST_ACK:
						TRACE("Last acknoledge")
						itsState=MC_STOP;
						onCompletion();
						break;

					case MC_SENDING_HEADER:
					case MC_SENDING_BLOCKS:
						TRACE("Send new block")
						sendBlock();
						break;
				}
				break;
				
			default:
				TRACE("Return value not allowed")
				itsState=MC_FAILED;
				onCompletion();		
	}
	
	TRACE("MemoryChannelClient::success - end")
}

void MemoryChannelClient::fail(string theError)
{
	TRACE("MemoryChannelClient::fail - start")
	itsState=MC_FAILED;
	onCompletion();	
	WARNING(theError.c_str())
	TRACE("MemoryChannelClient::fail - end")
}

void MemoryChannelClient::onWakeup(Wakeup* theMessage)
{
	TRACE("MemoryChannelClient::onWakeup - start")

	if(itsState!=MC_STOP && itsState!=MC_FAILED && Timer::time()-itsLastTx > MC_TIMEOUT)
	{
		WARNING("Timeout during transmition")
		itsState=MC_FAILED;
		onCompletion();
	}

	Client::onWakeup(theMessage);
	TRACE("MemoryChannelClient::onWakeup - end")
}

void MemoryChannelClient::markBlocks(unsigned long theDisplacement,unsigned long theSize)
{
	TRACE("MemoryChannelClient::markBlocks - start")

	unsigned long start=theDisplacement/MC_BLOCKSIZE;
	unsigned long end=(theDisplacement+theSize-1)/MC_BLOCKSIZE;
	TRACE("Displ=" << theDisplacement << " Size=" << theSize << " Start=" << start << " End=" << end)

	for(unsigned long cnt=start; cnt <= end; cnt++)
		itsMap[cnt]=MC_CHANGED;

	TRACE("MemoryChannelClient::markBlocks - end")
}

void MemoryChannelClient::set(unsigned long theDisplacement,long long theValue)
{
	TRACE("MemoryChannelClient::set - start")
	if(theDisplacement+sizeof(theValue)<=itsSize)
	{
		*(long long*)(itsBuffer+theDisplacement)=theValue;
		markBlocks(theDisplacement,sizeof(theValue));
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelClient::set - end")
}

void MemoryChannelClient::set(unsigned long theDisplacement,long theValue)
{
	TRACE("MemoryChannelClient::set - start")
	if(theDisplacement+sizeof(theValue)<=itsSize)
	{
		*(long*)(itsBuffer+theDisplacement)=theValue;	
		markBlocks(theDisplacement,sizeof(theValue));
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelClient::set - end")
}

void MemoryChannelClient::set(unsigned long theDisplacement,short theValue)
{
	TRACE("MemoryChannelClient::set - start")
	if(theDisplacement+sizeof(theValue)<=itsSize)
	{
		*(short*)(itsBuffer+theDisplacement)=theValue;	
		markBlocks(theDisplacement,sizeof(theValue));
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelClient::set - end")
}

void MemoryChannelClient::set(unsigned long theDisplacement,char theValue)
{
	TRACE("MemoryChannelClient::set - start")
	if(theDisplacement+sizeof(theValue)<=itsSize)
	{
		*(itsBuffer+theDisplacement)=theValue;	
		markBlocks(theDisplacement,sizeof(theValue));
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelClient::set - end")
}

void MemoryChannelClient::set(unsigned long theDisplacement,char* theSourcePtr,unsigned long theSize)
{
	TRACE("MemoryChannelClient::set - start")
	if(theDisplacement+theSize<=itsSize)
	{
		memcpy(itsBuffer+theDisplacement,theSourcePtr,theSize);	
		markBlocks(theDisplacement,theSize);
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelClient::set - end")
}

void MemoryChannelClient::set(unsigned long theDisplacement,unsigned long theSize,char theValue)
{
	TRACE("MemoryChannelClient::set - start")
	if(theDisplacement+theSize<=itsSize)
	{
		memset(itsBuffer+theDisplacement,theValue,theSize);	
		markBlocks(theDisplacement,theSize);
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelClient::set - end")
}

MemoryChannelServer::MemoryChannelServer(const char* theName)
				    :Server(theName)
{
	TRACE("MemoryChannelServer::MemoryChannelServer - start")
	itsBuffer=NULL;
	itsSize=0;
	itsSession.blocksReceived=0;
	itsSession.blocksToRx=0;
	itsSession.size=0;
	itsSession.token=0;
	itsSession.time=0;
	itsSession.buffer=NULL;
	itsSession.blocks=0;	
	itsSession.lastblock=0;	
	TRACE("MemoryChannelServer::MemoryChannelServer - end")
}

MemoryChannelServer::~MemoryChannelServer()
{
	TRACE("MemoryChannelServer::~MemoryChannelServer - start")
	if(itsBuffer!=NULL)
		delete [] itsBuffer;

	if(itsSession.buffer!=NULL)
		delete [] itsSession.buffer;
	TRACE("MemoryChannelServer::~MemoryChannelServer - end")
}

void MemoryChannelServer::copyBuffer()
{
	TRACE("MemoryChannelServer::copyBuffer - start")

	if(itsBuffer==NULL)
	{
		TRACE("Allocate buffer")
		itsSize=itsSession.size;
		itsBuffer=new char[itsSize];
		memcpy(itsBuffer,itsSession.buffer,itsSize);
	}
	else if(itsBuffer!=NULL && itsSize!=itsSession.size)
	{
		TRACE("Resize buffer")
		delete [] itsBuffer;
		itsSize=itsSession.size;
		itsBuffer=new char[itsSize];
		memcpy(itsBuffer,itsSession.buffer,itsSize);
	}
	else
	{
		TRACE("Copy only")
		memcpy(itsBuffer,itsSession.buffer,itsSize);
	}

	TRACE("MemoryChannelServer::copyBuffer - start")
}
	
string MemoryChannelServer::service(string theBuffer)
{
	TRACE("MemoryChannelServer::service - start")
	string ret;
	ListProperty aResponse;

	decodeProperties(theBuffer,itsRxStructure);

	Property* aProperty=itsRxStructure.get("BT");
	if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
	{
		string btype=((StringProperty*)aProperty)->get();
		TRACE("Block type=" << btype)

		if(btype.compare("HDR")==0)
		{			
			receiveHeader(aResponse);
		}
		else if(btype.compare("BLK")==0)
		{
			receiveBlock(aResponse);
		}
		else
		{
			setResponse(aResponse,MC_NOT_ALLOWED);
		}	
	}

	encodeProperties(aResponse,ret);

	TRACE("MemoryChannelServer::service - end")
	return ret;
}

void MemoryChannelServer::setResponse(ListProperty& theResponse,MemoryChannelResult theCode,unsigned long theToken)
{
	TRACE("MemoryChannelServer::setResponse - start")
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
	TRACE("MemoryChannelServer::setResponse - end")
}

void MemoryChannelServer::receiveHeader(ListProperty& theResponse)
{
	TRACE("MemoryChannelServer::receiveHeader - start")

	Property* aProperty;	
	itsSession.blocksToRx=0;
	itsSession.size=0;
	itsSession.time=Timer::time();	
	itsSession.token=RSHash(getName())+Timer::time();
	itsSession.blocksReceived=1;
	itsSession.lastblock=0;	

	aProperty=itsRxStructure.get("TB");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		itsSession.blocksToRx=((LongIntProperty*)aProperty)->get();
		TRACE("Blocks to rx=" << itsSession.blocksToRx)
	}

	aProperty=itsRxStructure.get("BS");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		itsSession.size=((LongIntProperty*)aProperty)->get();
		unsigned long blocks=itsSession.size/MC_BLOCKSIZE;
		blocks+=(itsSession.size%MC_BLOCKSIZE>0) ? 1 : 0;
	
		if(itsSession.buffer==NULL && itsSession.size>0)
		{
			TRACE("Allocate buffer")
			itsSession.blocks=blocks;
			itsSession.buffer=new char[itsSession.blocks*MC_BLOCKSIZE];
			memset(itsSession.buffer,0,itsSession.blocks*MC_BLOCKSIZE);	
		}
		else if(itsSession.buffer!=NULL && blocks!=itsSession.blocks && itsSession.size>0)
		{
			TRACE("Resize buffer")
			delete [] itsSession.buffer;
			itsSession.blocks=blocks;
			itsSession.buffer=new char[itsSession.blocks*MC_BLOCKSIZE];
			memset(itsSession.buffer,0,itsSession.blocks*MC_BLOCKSIZE);	
		}

		TRACE("Buffer size=" << itsSession.size)
		TRACE("Blocks=" << itsSession.blocks)
	}

	unsigned long bindex=0;
	aProperty=itsRxStructure.get("BI");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		bindex=((LongIntProperty*)aProperty)->get();
		TRACE("Block index=" << bindex)
	}

	aProperty=itsRxStructure.get("BF");
	if(aProperty!=NULL && aProperty->is(PROPERTY_STRING) && itsSession.size>0 && itsSession.blocksToRx>0)
	{
		string buffer=((StringProperty*)aProperty)->get();
		TRACE("Buffer size=" << buffer.size())

		if(buffer.size()==MC_BLOCKSIZE && bindex<itsSession.blocks)
		{
			memcpy(itsSession.buffer+bindex*MC_BLOCKSIZE,buffer.data(),MC_BLOCKSIZE);	
						
			if(itsSession.blocksToRx==1)
			{
				setResponse(theResponse,MC_SESSION_FINISHED,itsSession.token);
				copyBuffer();
				itsSession.blocks=0;
				itsSession.token=0;
				itsSession.lastblock=0;	
				onCompletion();
				TRACE("Session finished")
			}
			else
			{
				itsSession.lastblock=bindex;	
				setResponse(theResponse,MC_SESSION_STARTED,itsSession.token);
				TRACE("Session started. Token=" << itsSession.token)
			}
		}
		else
		{
			setResponse(theResponse,MC_NOT_ALLOWED);
			TRACE("Wrong parameters from peer")
		}
	}
	else
	{
		setResponse(theResponse,MC_NOT_ALLOWED);
		TRACE("Wrong parameters from peer")
	}

	TRACE("MemoryChannelServer::receiveHeader - end")
}

void MemoryChannelServer::receiveBlock(ListProperty& theResponse)
{
	TRACE("MemoryChannelServer::receiveBlock - start")

	Property* aProperty=itsRxStructure.get("TK");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		unsigned long token=((LongIntProperty*)aProperty)->get();
		TRACE("Session token=" << token)
		
		if(itsSession.token==token)
		{
			TRACE("Session found")
			
			unsigned long bindex=0;
			aProperty=itsRxStructure.get("BI");
			if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			{
				bindex=((LongIntProperty*)aProperty)->get();
				TRACE("Block index=" << bindex)
			}
		
			aProperty=itsRxStructure.get("BF");
			if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
			{
				string buffer=((StringProperty*)aProperty)->get();
				TRACE("Buffer size=" << buffer.size())
		
				if(buffer.size()==MC_BLOCKSIZE && bindex<itsSession.blocks)
				{
					memcpy(itsSession.buffer+bindex*MC_BLOCKSIZE,buffer.data(),MC_BLOCKSIZE);	

					if(bindex!=itsSession.lastblock)						
						itsSession.blocksReceived++;
								
					if(itsSession.blocksReceived>=itsSession.blocksToRx)
					{
						setResponse(theResponse,MC_SESSION_FINISHED,itsSession.token);
						copyBuffer();
						itsSession.blocks=0;
						itsSession.token=0;
						itsSession.lastblock=0;	
						onCompletion();
						TRACE("Session finished")
					}
					else
					{
						itsSession.lastblock=bindex;	
						setResponse(theResponse,MC_SESSION_CONTINUE,itsSession.token);
						TRACE("Session  continue. Token=" << itsSession.token)
					}
				}
				else
				{
					setResponse(theResponse,MC_NOT_ALLOWED);
					TRACE("Wrong parameters from peer")
				}
			}
			else
			{
				setResponse(theResponse,MC_NOT_ALLOWED);
				TRACE("Wrong parameters from peer")
			}
		}
		else
		{
			TRACE("Session not found")			
			setResponse(theResponse,MC_NOT_ALLOWED);
		}
	}
	else
	{
		TRACE("Wrong parameters from peer")
		setResponse(theResponse,MC_NOT_ALLOWED);
	}

	TRACE("MemoryChannelServer::receiveBlock - end")
}

void MemoryChannelServer::onWakeup(Wakeup* theMessage)
{
	TRACE("MemoryChannelServer::onWakeup - start")
	
	if(itsSession.token!=0 && Timer::time()-itsSession.time>MC_TIMEOUT)
	{
		TRACE("Session timeout. Token=" << itsSession.token << " dTime=" << (theMessage->getTime()-itsSession.time))
		WARNING("Session dropped for timeout")
		itsSession.blocksReceived=0;
		itsSession.blocksToRx=0;
		itsSession.size=0;
		itsSession.token=0;
		itsSession.time=0;
		itsSession.lastblock=0;	
	}			

	TRACE("MemoryChannelServer::onWakeup - end")
}	

long long MemoryChannelServer::get64(unsigned long theDisplacement)
{
	TRACE("MemoryChannelServer::get64 - start")
	long long res=0;
	if(theDisplacement+sizeof(res)<=itsSize)
	{
		res=*(long long*)(itsBuffer+theDisplacement);
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelServer::get64 - end")
	return res;
}

long MemoryChannelServer::get32(unsigned long theDisplacement)
{
	TRACE("MemoryChannelServer::get32 - start")
	long res=0;
	if(theDisplacement+sizeof(res)<=itsSize)
	{
		res=*(long*)(itsBuffer+theDisplacement);
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelServer::get32 - end")
	return res;
}

short MemoryChannelServer::get16(unsigned long theDisplacement)
{
	TRACE("MemoryChannelServer::get16 - start")
	short res=0;
	if(theDisplacement+sizeof(res)<=itsSize)
	{
		res=*(short*)(itsBuffer+theDisplacement);
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelServer::get16 - end")
	return res;
}

char MemoryChannelServer::get8(unsigned long theDisplacement)
{
	TRACE("MemoryChannelServer::get8 - start")
	char res=0;
	if(theDisplacement+sizeof(res)<=itsSize)
	{
		res=*(itsBuffer+theDisplacement);
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelServer::get8 - end")
	return res;
}

void MemoryChannelServer::get(unsigned long theDisplacement,char* theDestPtr,unsigned long theSize)
{
	TRACE("MemoryChannelServer::get - start")
	if(theDisplacement+theSize<=itsSize)
	{
		memcpy(theDestPtr,itsBuffer+theDisplacement,theSize);	
	}
	else
	{
		throw MemoryChannelException(MEMORY_OVERFLOW);
	}		
	TRACE("MemoryChannelServer::get - end")
}


