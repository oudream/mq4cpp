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

#include "RequestReply.h"
#include "Logger.h"
#include "Compression.h"
#include "Timer.h"
#include "FileSystem.h"
#include "FileTransfer.h"
#include "Router.h"
#include <string>
#include <strstream>
#include <signal.h>
#include <stdio.h>

#ifdef  WIN32
#include <crtdbg.h>
#else
#endif

using namespace std;

#define TEST1_MESSAGES 2000
#define TEST2_MESSAGES 2000
#define TEST3_FILESIZE 1000000
#define PACKETSIZE 256
#define TEMPDIR "temp"
#define PASSWORD "MyVerySecretPassword"
#define TEST_REQUESTREPLY
#define TEST_PUBLISHSUBSCRIBE
#define TEST_FTP
//#define TEST_PARALLEL
#define RAMPUP 1000

/////////////////////////////////////////////////////////////////////////////
// T E S T # 1 - Request/Reply
//

class Test1Client : public Client
{
protected:
	unsigned long itsMsgCnt;
	_TIMEVAL itsStartTime;	
	_TIMEVAL itsEndTime;	
	
public:
	bool finished;

	Test1Client(const char* theName, char* theHost,int thePort, const char* theTarget,
			    bool ENCRIPTION,bool COMPRESSION,bool CACHE)
		: Client(theName,theHost,thePort,theTarget)
	{
		char buffer[128];
		sprintf(buffer,"Test1Client Request/Reply: Encription=%s, Compression=%s, Cache=%s",
			    ((ENCRIPTION)? "TRUE":"FALSE"),((COMPRESSION)? "TRUE":"FALSE"),((CACHE)? "TRUE":"FALSE"));
		LOG(buffer)
		if(ENCRIPTION) setEncription(new Rijndael256(Encription::generateKey256(PASSWORD))); 
		if(COMPRESSION) setCompression(new PacketCompression(CACHE));
		itsMsgCnt=0;
		itsStartTime=Timer::timeExt();
		finished=false;
		send(generate());		
	};
	
	virtual ~Test1Client() {};

protected:
	virtual string generate()
	{
		string buffer;
		buffer.reserve(PACKETSIZE);

		for(int cnt=0; cnt < PACKETSIZE ; cnt++)
			buffer += (unsigned char)(cnt & 0x07);

		itsMsgCnt++;
		return buffer;				
	};

	void printResults()
	{
		itsEndTime=Timer::timeExt();
		long delta=Timer::subtractMillisecs(&itsStartTime,&itsEndTime);
		float msgrate=(float)TEST1_MESSAGES*1000.0/(float)delta;
		float datarate=(float)TEST1_MESSAGES*(float)PACKETSIZE*1000.0/(float)delta;
		char buffer[128];
		sprintf(buffer,"Test1 result: elapsed %u ms, messages rate %1.0f pck/s, data rate %1.0f byte/s",delta,msgrate,datarate);
		LOG(buffer)
		finished=true;
	};
	
	void success(string theBuffer)
	{
		if(itsMsgCnt<TEST1_MESSAGES)
			send(generate());
		else
			printResults();
	};
	
	void fail(string theError)
	{
		WARNING("Test1Client Request/Reply service failed")		
		if(itsMsgCnt<TEST1_MESSAGES)
			send(generate());		
		else
			printResults();
	};
};

class Test1Server : public Server
{
public:
	Test1Server(const char* theName,bool ENCRIPTION,bool COMPRESSION,bool CACHE) : Server(theName) 
	{
		char buffer[128];
		sprintf(buffer,"Test1Server Request/Reply: Encription=%s, Compression=%s, Cache=%s",
			    ((ENCRIPTION)? "TRUE":"FALSE"),((COMPRESSION)? "TRUE":"FALSE"),((CACHE)? "TRUE":"FALSE"));
		LOG(buffer)
		if(ENCRIPTION) setEncription(new Rijndael256(Encription::generateKey256(PASSWORD))); 
		if(COMPRESSION) setCompression(new PacketCompression(CACHE));
	};	
	
	virtual ~Test1Server() {};

protected:
	string service(string theBuffer)
	{
		return "OK";
	};
};

/////////////////////////////////////////////////////////////////////////////
// T E S T # 2 - Publish/Subscribe
//

class Test2Client : public Client
{
protected:
	int itsCheckCnt;
	unsigned int itsMsgRcved;
	
public:
	bool finished;

	Test2Client(const char* theName, char* theHost,int thePort, const char* theTarget,
			    bool ENCRIPTION,bool COMPRESSION,const char* theTopic)
		: Client(theName,theHost,thePort,theTarget)
	{
		char buffer[128];
		sprintf(buffer,"Test2Client Publish/Subscribe: Encription=%s, Compression=%s",
			    ((ENCRIPTION)? "TRUE":"FALSE"),((COMPRESSION)? "TRUE":"FALSE"));
		LOG(buffer)
		if(ENCRIPTION) setEncription(new Rijndael256(Encription::generateKey256(PASSWORD))); 
		if(COMPRESSION) setCompression(new PacketCompression(false));
		itsMsgCnt=0;
		finished=false;
		itsCheckCnt=0;
		itsMsgRcved=0;
		
		for(int i=0; i < TEST2_MESSAGES; i++)
		{
			publish(theTopic,generate());
#ifndef WIN32
			Thread::sleep(1);
#endif
		}		
	};
	
	virtual ~Test2Client() {};

protected:
	void onWakeup(Wakeup* theMessage) // Polling each 500ms
	{
		if(!finished && ++itsCheckCnt>20)
		{
			finished=true;
			char buffer[256];
			sprintf(buffer,"Test2 result: failed. Received only %u messages",itsMsgRcved);
			LOG(buffer)
		}

		if(!finished)
			send("CHECK");

		Client::onWakeup(theMessage);
	};

	virtual string generate()
	{
		string buffer;
		buffer.reserve(PACKETSIZE);

		for(int cnt=0; cnt < PACKETSIZE ; cnt++)
			buffer += (unsigned char)(cnt & 0x07);

		return buffer;				
	};

	void success(string theBuffer)
	{
		if(!(theBuffer.substr(0,3).compare("NOK")==0))
		{
			unsigned delta=0L;
			sscanf(theBuffer.c_str(),"%u",&delta);
			float msgrate=(float)TEST2_MESSAGES*1000.0/(float)delta;
			float datarate=(float)TEST2_MESSAGES*(float)PACKETSIZE*1000.0/(float)delta;
			finished=true;
			char buffer[128];
			sprintf(buffer,"Test2 result: elapsed %u ms, messages rate %1.0f pck/s, data rate %1.0f byte/s",delta,msgrate,datarate);
			LOG(buffer)
		}
		else
		{
			sscanf(theBuffer.substr(3,5).c_str(),"%u",&itsMsgRcved);
		}
	};
	
	void fail(string theError)
	{
		WARNING("Test2Cient Request/Reply service failed")		
	};
};

class Test2Server : public Server
{
protected:
	int itsMsgCnt;
	bool itsFirstArrived;	
	_TIMEVAL itsStartTime;	
	_TIMEVAL itsEndTime;	

public:
	Test2Server(const char* theName,bool ENCRIPTION,bool COMPRESSION,const char* theTopic) : Server(theName) 
	{
		char buffer[128];
		sprintf(buffer,"Test2Server Publish/Subscribe: Encription=%s, Compression=%s",
			    ((ENCRIPTION)? "TRUE":"FALSE"),((COMPRESSION)? "TRUE":"FALSE"));
		LOG(buffer)
		if(ENCRIPTION) setEncription(new Rijndael256(Encription::generateKey256(PASSWORD))); 
		if(COMPRESSION) setCompression(new PacketCompression(false));
		subscribe(theTopic);
		itsMsgCnt=0;
		itsFirstArrived=false;		
	};	
	
	virtual ~Test2Server() {};

protected:
	string service(string theBuffer)
	{
		if(theBuffer.substr(0,5).compare("CHECK")==0 && itsMsgCnt>=TEST2_MESSAGES)
		{
			long delta=Timer::subtractMillisecs(&itsStartTime,&itsEndTime);
			char buffer[128];
			sprintf(buffer,"%u",delta);
			return string(buffer);
		}
		else
		{				
			char buffer[128];
			sprintf(buffer,"NOK %u",itsMsgCnt);
			return string(buffer);
		}

		return "";
	};
	
	virtual void onBroadcast(NetworkMessage* theMessage)
	{
		if(!itsFirstArrived)
		{
			itsFirstArrived=true;	
			itsStartTime=Timer::timeExt();
		}

		if(++itsMsgCnt>=TEST2_MESSAGES)
		{
			itsEndTime=Timer::timeExt();
		}
	};
};

/////////////////////////////////////////////////////////////////////////////
// T E S T # 3 - File transfer 
//

class Test3Client : public FileTransferClient
{
protected:
	_TIMEVAL itsStartTime;	
	_TIMEVAL itsEndTime;	
	
public:
	bool finished;

	Test3Client(const char* theName, char* theHost,int thePort, const char* theTarget,
			    bool ENCRIPTION,bool COMPRESSION,bool CACHE)
		: FileTransferClient(theName,theHost,thePort,theTarget)
	{
		char buffer[128];
		sprintf(buffer,"Test3Client File transfer: Encription=%s, Compression=%s, Cache=%s",
			    ((ENCRIPTION)? "TRUE":"FALSE"),((COMPRESSION)? "TRUE":"FALSE"),((CACHE)? "TRUE":"FALSE"));
		LOG(buffer)
		if(ENCRIPTION) setEncription(new Rijndael256(Encription::generateKey256(PASSWORD))); 
		if(COMPRESSION) setCompression(new PacketCompression(CACHE));
		itsStartTime=Timer::timeExt();
		finished=false;
	};
	
	virtual ~Test3Client() {};

protected:
	virtual void onCompletion()
	{
		if(!fail())
		{
			itsEndTime=Timer::timeExt();
			long delta=Timer::subtractMillisecs(&itsStartTime,&itsEndTime);
			float msgrate=((float)TEST3_FILESIZE*1000.0)/((float)delta*(float)FT_BLOCKSIZE);
			float datarate=(float)TEST3_FILESIZE*1000.0/(float)delta;
			char buffer[128];
			sprintf(buffer,"Test3 result: elapsed %u ms, messages rate %1.0f pck/s, data rate %1.0f byte/s",delta,msgrate,datarate);
			LOG(buffer)
		}
		else
		{
			LOG("Test failed")
		}			
			
		finished=true;
	};	
};

/////////////////////////////////////////////////////////////////////////////
// M A I N
//

MessageProxyFactory* itsFactory=NULL;
enum
{
	NONE,
	CLIENT,
	ROUTER,
	SERVER,
	LOCALROUTER,
} state=NONE;

const char* getName()
{
	return "Main";	
}

void shutdown()
{
	LOG("Shutdown in progress")
	Thread::sleep(100);
	Thread::shutdownInProgress();

	if(itsFactory!=NULL)
		delete itsFactory;

	STOPLOGGER()
	STOPTIMER()
	STOPREGISTRY()

#ifdef WIN32
	/* DUMP MEMORY LEAK INFORMATION */
	_CrtDumpMemoryLeaks () ;
#endif

	DISPLAY("Program terminated")
	if(state==CLIENT)    
    	DISPLAY("See client.log for details")
	else if(state==ROUTER)    
    	DISPLAY("See router.log for details")
    else	
    	DISPLAY("See server.log for details")	
	exit(0);	
}

void signalcallback(int a)
{
	WARNING("Program terminated by user request")
	shutdown();	
}

typedef struct _Test1Config_
{
	char* client;
	char* server;
	bool encription;
	bool compression;
	bool cache;
} Test1Config; 

Test1Config Conf1[5]=
{
	{ "Test1C_FFF","Test1S_FFF",false,false,false },
	{ "Test1C_TFF","Test1S_TFF",true,false,false },
	{ "Test1C_FTF","Test1S_FTF",false,true,false },
	{ "Test1C_FTT","Test1S_FTT",false,true,true },
	{ "Test1C_TTT","Test1S_TTT",true,true,true }
};

typedef struct _Test2Config_
{
	char* client;
	char* server;
	char* topic;
	bool encription;
	bool compression;
	bool cache;
} Test2Config; 

Test2Config Conf2[4]=
{
	{ "Test2P_FF","Test2S_FF","Topic1",false,false },
	{ "Test2P_TF","Test2S_TF","Topic2",true,false },
	{ "Test2P_FT","Test2S_FT","Topic3",false,true },
	{ "Test2P_TT","Test2S_TT","Topic4",true,true}
};

Test1Config Conf3[5]=
{
	{ "fileclient1","fileserver1",false,false,false },
	{ "fileclient2","fileserver2",true,false,false },
	{ "fileclient3","fileserver3",false,true,false },
	{ "fileclient4","fileserver4",false,true,true },
	{ "fileclient5","fileserver5",true,true,true }
};

char* TempFileName[5]=
{
	"Temp1.log",
	"Temp2.log",
	"Temp3.log",
	"Temp4.log",
	"Temp5.log"
};

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP - Message Queuing for C++")
	DISPLAY("LGPL Copyright(C) 2004-2007 Riccardo Pompeo (Italy)")
	DISPLAY("Details of this project at http://www.sixtyfourbit.org")
	DISPLAY("This application allows to bench MQ4CPP on your platform")	
	
	char* host=NULL;
	int hport=0;
	int rport=0;
	
	if(argv < 3)
	{
		DISPLAY("Client usage: benchmark -c hostip port")
		DISPLAY("Router usage: benchmark -r port hostip port")
		DISPLAY("Server usage: benchmark -s port")
		DISPLAY("Server with local router usage: benchmark -l port")
		return 0;	
	}
	else if(string(argc[1]).compare("-c")==0 && argv==4)
	{
		state=CLIENT;
		DISPLAY("Host name=" << argc[2])
		host=argc[2];
		DISPLAY("Host port=" << argc[3])
		hport=atoi(argc[3]);
	}	
	else if(string(argc[1]).compare("-r")==0 && argv==5)
	{
		state=ROUTER;
		DISPLAY("Server port=" << argc[2])
		rport=atoi(argc[2]);
		DISPLAY("Host name=" << argc[3])
		host=argc[3];
		DISPLAY("Host port=" << argc[4])
		hport=atoi(argc[4]);
	}	
	else if(string(argc[1]).compare("-s")==0 && argv==3)
	{
		state=SERVER;
		DISPLAY("Server port=" << argc[2])
		hport=atoi(argc[2]);
	}	
	else if(string(argc[1]).compare("-l")==0 && argv==3)
	{
		state=LOCALROUTER;
		DISPLAY("Server port=" << argc[2])
		hport=atoi(argc[2]);
	}	
	else
	{
		DISPLAY("Client usage: benchmark -c hostip port")
		DISPLAY("Router usage: benchmark -r port hostip port")
		DISPLAY("Server usage: benchmark -s port")
		DISPLAY("Server with local router usage: benchmark -l port")
		return 0;	
	}

	signal(SIGABRT,signalcallback);
	signal(SIGINT,signalcallback);
	signal(SIGTERM,signalcallback);

	try
	{
		if(state==CLIENT)
		{
	    	DISPLAY("Starting client threads...")
			STARTLOGGER("client.log")
			LOG("Start benchmark")
			char buffer[128];
			sprintf(buffer,"Parameters: packet size %u, messages %u",PACKETSIZE,TEST1_MESSAGES);
			LOG(buffer)

			Test1Client* aClient1[5];
			for(int i=0; i < 5; i++) aClient1[i]=NULL; 
			Test2Client* aClient2[4];
			for(int i=0; i < 4; i++) aClient2[i]=NULL; 
			Test3Client* aClient3[5];
			for(int i=0; i < 5; i++) aClient3[i]=NULL; 

#ifdef TEST_REQUESTREPLY
			DISPLAY("Test 1 rampup started")
			for(int i=0; i < 5; i++)
			{
				aClient1[i]=new Test1Client(Conf1[i].client,host,hport,Conf1[i].server,
														Conf1[i].encription,Conf1[i].compression,Conf1[i].cache);
#ifdef TEST_PARALLEL
				Thread::sleep(RAMPUP);
#else
				while(!aClient1[i]->finished) {	Thread::sleep(100);	}
#endif
			}
#endif

#ifdef TEST_PUBLISHSUBSCRIBE
			DISPLAY("Test 2 rampup started")
			for(int i=0; i < 4; i++)
			{
				aClient2[i]=new Test2Client(Conf2[i].client,host,hport,Conf2[i].server,
														Conf2[i].encription,Conf2[i].compression,Conf2[i].topic);
#ifdef TEST_PARALLEL
				Thread::sleep(RAMPUP);
#else
				while(!aClient2[i]->finished) {	Thread::sleep(100);	}
#endif
			}
#endif

#ifdef TEST_FTP
			DISPLAY("Test 3 rampup started")
			Directory* CurDir=Directory::getCurrent();
			File* TempFile[5];
			TempFile[0]=(File*)CurDir->get(TempFileName[0]);
			if(TempFile[0]==NULL)
			{
				TempFile[0]=CurDir->create(TempFileName[0]);
				fstream& aStream=TempFile[0]->create();
				for(int i=0; i < TEST3_FILESIZE; i++)
					aStream << (char)(rand()&0x1f);
				TempFile[0]->close();
				DISPLAY(TempFile[0]->getName() << " created")
				string aDirName=CurDir->encodeFullName();
						
				for(int i=1; i < 5; i++)
				{
					string aFileName=aDirName+"/"+TempFileName[i];
					TempFile[i]=TempFile[0]->copy(aFileName.c_str());
					DISPLAY(aFileName << " created")		
				}
			}
			else
			{
				for(int i=1; i < 5; i++)
				{
					TempFile[i]=(File*)CurDir->get(TempFileName[i]);
				}
			}	

			for(int i=0; i < 5; i++)
			{
				aClient3[i]=new Test3Client(Conf3[i].client,host,hport,Conf3[i].server,
														Conf3[i].encription,Conf3[i].compression,Conf3[i].cache);

				aClient3[i]->send(TempFile[i]);

#ifdef TEST_PARALLEL
				Thread::sleep(RAMPUP);
#else
				while(!aClient3[i]->finished) {	Thread::sleep(100);	}
#endif
			}

			delete CurDir;
#endif

#ifdef TEST_PARALLEL
			DISPLAY("Rampup completed")
			bool finished;
			do
			{
				Thread::sleep(100);
				finished=true;

				for(int i=0; i < 5; i++) 
					if(aClient1[i]!=NULL)
						finished=aClient1[i]->finished && finished; 

				for(int i=0; i < 4; i++) 
					if(aClient2[i]!=NULL)
						finished=aClient2[i]->finished && finished; 

				for(int i=0; i < 5; i++)
					if(aClient3[i]!=NULL)
						finished=aClient3[i]->finished && finished; 
				
			}  while(!finished);
#endif						
			DISPLAY("Test completed")
			LOG("End benchmark")
		}
		else if(state==ROUTER)
		{
	    	DISPLAY("Starting router threads...")
			STARTLOGGER("router.log")
			LOG("Start benchmark router")
			itsFactory=new MessageProxyFactory("BenchmarkRouterFactory",rport);
			RemoteRouter* aRouter=NULL;
			
			for(int i=0; i < 5; i++)
			{
				aRouter=new RemoteRouter(Conf1[i].server, host,hport,Conf1[i].server);
			}
			
			for(int i=0; i < 4; i++)
			{
				aRouter=new RemoteRouter(Conf2[i].server, host,hport,Conf2[i].server);
			}

			for(int i=0; i < 5; i++)
			{
				aRouter=new RemoteRouter(Conf3[i].server, host,hport,Conf3[i].server);
			}

	    	DISPLAY("Router threads are running. Press CTRL-C to exit.")
			while(1)
			{
				Thread::sleep(1000);
			}
		}
		else if(state==LOCALROUTER)
		{
	    	DISPLAY("Starting server and router threads...")
			STARTLOGGER("server.log")
			LOG("Start benchmark server")
			itsFactory=new MessageProxyFactory("BenchmarkServerFactory",hport);
			LocalRouter* aRouter=NULL;
			string aTarget;
			
			for(int i=0; i < 5; i++)
			{
				aTarget="_"+string(Conf1[i].server)+"_";
				Test1Server* aServer1=new Test1Server(aTarget.c_str(),
												Conf1[i].encription,Conf1[i].compression,Conf1[i].cache);
				aRouter=new LocalRouter(Conf1[i].server,aTarget.c_str());
			}
			
			for(int i=0; i < 4; i++)
			{
				aTarget="_"+string(Conf2[i].server)+"_";
				Test2Server* aServer2=new Test2Server(aTarget.c_str(),
												Conf2[i].encription,Conf2[i].compression,Conf2[i].topic);
				aRouter=new LocalRouter(Conf2[i].server, aTarget.c_str());
			}

			Directory* CurDir=Directory::getCurrent();
			Directory* TempDir=(Directory*)CurDir->get(TEMPDIR);
			if(TempDir==NULL)
			{
				TempDir=CurDir->mkdir(TEMPDIR);
				DISPLAY(TempDir->getName() << " created")		
			}	

			aTarget="_"+string(Conf3[0].server)+"_";
			FileTransferServer* aServer1=new FileTransferServer(aTarget.c_str(),TempDir);

			aTarget="_"+string(Conf3[1].server)+"_";
			FileTransferServer* aServer2=new FileTransferServer(aTarget.c_str(),TempDir,
												new Rijndael256(Encription::generateKey256(PASSWORD)));

			aTarget="_"+string(Conf3[2].server)+"_";
			FileTransferServer* aServer3=new FileTransferServer(aTarget.c_str(),TempDir,
												new PacketCompression(false));

			aTarget="_"+string(Conf3[3].server)+"_";
			FileTransferServer* aServer4=new FileTransferServer(aTarget.c_str(),TempDir,
												new PacketCompression(true));
	
			aTarget="_"+string(Conf3[4].server)+"_";
			FileTransferServer* aServer5=new FileTransferServer(aTarget.c_str(),TempDir,
												new Rijndael256(Encription::generateKey256(PASSWORD)),
												new PacketCompression(true));

			delete CurDir;

			for(int i=0; i < 5; i++)
			{
				aTarget="_"+string(Conf3[i].server)+"_";
				aRouter=new LocalRouter(Conf3[i].server, aTarget.c_str());
			}

	    	DISPLAY("Router and server threads are running. Press CTRL-C to exit.")
			while(1)
			{
				Thread::sleep(1000);
			}
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
			STARTLOGGER("server.log")
			LOG("Start benchmark server")
			itsFactory=new MessageProxyFactory("BenchmarkServerFactory",hport);

			for(int i=0; i < 5; i++)
			{
				Test1Server* aServer1=new Test1Server(Conf1[i].server,
												Conf1[i].encription,Conf1[i].compression,Conf1[i].cache);

			}
			
			for(int i=0; i < 4; i++)
			{
				Test2Server* aServer2=new Test2Server(Conf2[i].server,
												Conf2[i].encription,Conf2[i].compression,Conf2[i].topic);
			}

			Directory* CurDir=Directory::getCurrent();
			Directory* TempDir=(Directory*)CurDir->get(TEMPDIR);
			if(TempDir==NULL)
			{
				TempDir=CurDir->mkdir(TEMPDIR);
				DISPLAY(TempDir->getName() << " created")		
			}	

			FileTransferServer* aServer1=new FileTransferServer(Conf3[0].server,TempDir);

			FileTransferServer* aServer2=new FileTransferServer(Conf3[1].server,TempDir,
												new Rijndael256(Encription::generateKey256(PASSWORD)));

			FileTransferServer* aServer3=new FileTransferServer(Conf3[2].server,TempDir,
												new PacketCompression(false));

			FileTransferServer* aServer4=new FileTransferServer(Conf3[3].server,TempDir,
												new PacketCompression(true));
	
			FileTransferServer* aServer5=new FileTransferServer(Conf3[4].server,TempDir,
												new Rijndael256(Encription::generateKey256(PASSWORD)),
												new PacketCompression(true));

			delete CurDir;

	    	DISPLAY("Server threads are running. Press CTRL-C to exit.")
			while(1)
			{
				Thread::sleep(1000);
			}
		}

	    DISPLAY("...stopping threads...")
		shutdown();
	}
	catch(Exception& ex) 
	{
		TRACE(ex.getMessage().c_str())
	}
	catch(...)
	{
		TRACE("Unhandled exception")	
	}		

#ifdef WIN32
	/* DUMP MEMORY LEAK INFORMATION */
	_CrtDumpMemoryLeaks () ;
#endif

    DISPLAY("...done!")
	if(state==CLIENT)    
    	DISPLAY("See client.log for details")
	else if(state==ROUTER)    
    	DISPLAY("See router.log for details")
    else	
    	DISPLAY("See server.log for details")
	return 0;
}
