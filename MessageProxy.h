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

#ifndef __MESSAGEPROXY__
#define __MESSAGEPROXY__

#include "Socket.h"
#include "MessageQueue.h"
#include "Vector.h"
#include "Timer.h"
#include "Encription.h"
#include "Compression.h"
#include "Properties.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/errno.h>
#endif

#include <vector>

#define MESSAGEPROXYHEADER "MessageProxy("

enum NetworkMessages
{
	MQ_PROXY_MESSAGE=1,
	MQ_PROXY_LOOKUP_REQUEST,
	MQ_PROXY_LOOKUP_REPLY,
	MQ_PROXY_PING_REQUEST,
	MQ_PROXY_PING_REPLY,	
	MQ_PROXY_UNSOLICITED,
	MQ_PROXY_BROADCAST	
};

class NetworkMessage : public Message
{
public:	
	typedef struct NetwokMessageStruct
	{
		MQHANDLE sender;
		unsigned short seqnum; 
		unsigned short topiclen;
		unsigned short buflen;	
	} NetworkMessageHeader;	
	
protected:
	string itsTopic;
	string itsBuffer;
	MQHANDLE itsTarget;	
	MQHANDLE itsRemoteSender;
	unsigned short itsSeqNum;
	bool itsUnsolicitedFlag;
	bool itsBroadcastFlag;
	
public:
	NetworkMessage(NetworkMessage& o);
	NetworkMessage(char* theBuffer, unsigned short theLen); 
	NetworkMessage(string theBuffer);
	virtual ~NetworkMessage() {};
	virtual Message* clone() { return new NetworkMessage(*this); };
	
	string getTopic() { return itsTopic; };
	void setTopic(string theTopic) { itsTopic=theTopic; };
	void setTopic(const char* theTopic) { itsTopic=theTopic; };  
	void setTopic(char* theTopic,int len) { itsTopic.assign(theTopic,len); };
	void setRemoteSender(MQHANDLE theHandle) { itsRemoteSender=theHandle; }; 
	MQHANDLE getRemoteSender() { return itsRemoteSender; }; 
	void setTarget(MQHANDLE theHandle) { itsTarget=theHandle; };
	MQHANDLE getTarget() { return itsTarget; };
	void setSequenceNumber(unsigned short theSequence) { itsSeqNum=theSequence; }; 
	unsigned short getSequenceNumber() { return itsSeqNum; }; 
	bool isUnsolicited() { return itsUnsolicitedFlag; };
	void setUnsolicited() { itsUnsolicitedFlag=true; };
	bool isBroadcasting() { return itsBroadcastFlag; };
	void setBroadcasting() { itsBroadcastFlag=true; };
	string get() { return itsBuffer; };
	virtual string toString(); 
	virtual void toStream(ostream& theStream);
	virtual void code(Encription* theEncr);
	virtual void decode(Encription* theEncr);
	virtual void inflate(Compression* theCompr);
	virtual void deflate(Compression* theCompr);
};

class PingRequestMessage : public Message
{
public:	
	typedef struct PingRequestStruct
	{
		MQHANDLE sender;
	} PingRequest;	
			
public:
	PingRequestMessage(MQHANDLE theSenderID);
	virtual ~PingRequestMessage() {};
	virtual string toString();
};

class PingReplyMessage : public Message
{
protected:
	MQHANDLE itsTarget;
	
public:
	PingReplyMessage(MQHANDLE theTarget);
	virtual ~PingReplyMessage() {};
	virtual MQHANDLE getTarget() { return itsTarget; };
};

class LookupRequestMessage : public Message
{
public:	
	typedef struct LookupRequestStruct
	{
		MQHANDLE sender;
		unsigned short namelen;	
	} LookupRequest;	
	
protected:
	string itsNameToLookup;
		
public:
	LookupRequestMessage(const char* theName,MQHANDLE theSenderID);
	virtual ~LookupRequestMessage() {};
	virtual string toString();
	virtual string getTarget () { return itsNameToLookup; };
};

class LookupReplyMessage : public Message
{
public:
	typedef struct LookupStruct
	{
		bool fail;
		MQHANDLE handle;
	} LookupReply;	

protected:
	
	LookupReply itsBuffer;
	MQHANDLE itsTarget;
	
public:
	LookupReplyMessage();
	LookupReplyMessage(MQHANDLE theTarget);
	LookupReplyMessage(MQHANDLE theTarget,MQHANDLE theHandle);
	LookupReplyMessage(LookupReply& theReply);
	virtual ~LookupReplyMessage() {};
	virtual string toString();
	virtual bool isFailed() { return itsBuffer.fail; };
	virtual MQHANDLE getHandle() { return itsBuffer.handle; };
	virtual MQHANDLE getTarget() { return itsTarget; };
};

class Observer : public MessageQueue
{
protected:	
	Encription* itsEncription;
	Compression* itsCompression;
	vector<string> itsTopicList;
	MQHANDLE itsLastMessageProxy;
	string itsLastReceivedTopic;
	
public:
	Observer(const char* theName);
	virtual ~Observer();
	virtual void setEncription(Encription* theEncr);
	virtual void setCompression(Compression* theCompr);

protected:
	virtual void post(MQHANDLE theTarget,NetworkMessage* theMessage);
	virtual void publish(string theTopic,string theMessage); // ++ v1.5
	virtual void subscribe(string theTopic) { itsTopicList.push_back(theTopic); }; // ++ v1.5

	virtual void onMessage(Message* theMessage);
	virtual void onWakeup(Wakeup* theMessage) {};
	virtual void onPing(PingReplyMessage* theMessage) {};
	virtual void onLookup(LookupReplyMessage* theMessage) {};
	virtual void onBroadcast(NetworkMessage* theMessage) {}; // ++ v1.5
	virtual void onUnsolicited(NetworkMessage* theMessage) {};
	virtual NetworkMessage* onRequest(NetworkMessage* theMessage) { return NULL; };
	virtual void onLocal(Message* theMessage) {};
	
	virtual void decodeProperties(string& theBuffer,ListProperty& theProperties);
	virtual void decodeProperties(char* theBuffer,unsigned long theLen,ListProperty& theProperties);	
	virtual void encodeProperties(ListProperty& theProperties,string& theBuffer);
};

class MessageProxy : public MessageQueue
{
protected:
	Socket* itsSocket;
	
	typedef struct PacketHeaderStruct
	{
		unsigned short sync;
		unsigned short type;		
		unsigned short target;
		unsigned short msglen;
	} header;
	
#ifdef WIN32	
	unsigned long* m_hThreadRx;
#else
	pthread_t m_hThreadRx;
#endif
	
public:
	MessageProxy(const char* theName);
	MessageProxy(const char* theName,Socket* theSocket);
	virtual ~MessageProxy();
	virtual void receive();
	virtual string getConnectionAddress(MQHANDLE theCaller,int& thePort);

protected:
	virtual void onMessage(Message* theMessage);
};

class MessageProxyFactory : public Thread, protected SocketServer
{
protected:
	unsigned long itsCount;
	unsigned itsPort;
	static Thread itsMutex;  // ++ v1.5

public:
	MessageProxyFactory(const char* theFactoryName,int theSocket);
	~MessageProxyFactory();
	static void ping(const char* theHost, unsigned thePort,MessageQueue* theSourceQueue);
	static void lookupAt(const char* theHost, unsigned thePort,
						 const char* theRemoteQueueName,MessageQueue* theSourceQueue);
	static string getUniqueNetID();
	
protected:
	void run();
	static void post(const char* theHost, unsigned thePort,Message* theMessage,MQHANDLE theSender);
	virtual void onNewConnection(string theAddress,unsigned short thePort) {};
};

extern "C" {
#ifdef WIN32	
	unsigned int _mp_thread_proc(void* param);
#else
	void* _mp_thread_proc(void* param);
#endif
}

#endif
