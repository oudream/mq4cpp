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
// History:
// Original Win32 code by Vijay R. Nyffenegger (2004)
// Modified to support Linux socket by Riccardo Pompeo (2004-2007)
// 


#ifndef __SOCKET__
#define __SOCKET__

#ifdef WIN32
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
typedef int SOCKET;
#endif

#include "Exception.h"
#include <string>
#include <vector>

enum TypeSocket {BlockingSocket, NonBlockingSocket};

class NetAdapter
{
protected:
	string itsName;  
	string itsIP;
	string itsMAC;

public:
	NetAdapter() {};

	NetAdapter(const NetAdapter& o)
	{
		itsName=o.itsName;
		itsIP=o.itsIP;
		itsMAC=o.itsMAC;
	};

	NetAdapter(string& name,string& ip,string& mac)
	{
		itsName=name;
		itsIP=ip;
		itsMAC=mac;
	};
	
	virtual ~NetAdapter() {};

	virtual const NetAdapter& operator=(const NetAdapter& o)
	{
		itsName=o.itsName;
		itsIP=o.itsIP;
		itsMAC=o.itsMAC;
		return o;
	};
	
	virtual string getName() { return itsName; };
	virtual string getIP() { return itsIP; };
	virtual string getMAC() { return itsMAC; };
};

class Socket 
{
public:

  virtual ~Socket();
  Socket(const Socket&);
  Socket& operator=(Socket&);

  bool ReceiveBuffer(void* theBuffer,int theLen);
  void SendBuffer(void* theBuffer,int theLen);

  string ReceiveLine();
  string ReceiveBytes();

  void Close();

  // The parameter of SendLine is not a const reference
  // because SendLine modifes the std::string passed.
  void SendLine(std::string);

  // The parameter of SendBytes is a const reference
  // because SendBytes does not modify the std::string passed 
  // (in contrast to SendLine).
  void SendBytes(const std::string&);
  
  static vector<NetAdapter>* getAdapters();

protected:
  friend class SocketServer;
  friend class SocketSelect;

  Socket(SOCKET s);
  Socket();

  SOCKET s_;

  int* refCounter_;
  
private:
  static void Start();
  static void End();
  static int  nofSockets_;
};

class SocketClient : public Socket 
{
public:
  SocketClient(const std::string& host, int port);
};

class SocketServer : public Socket 
{
protected:
  struct sockaddr itsSocketAddr;
	
public:
  SocketServer(int port, int connections, TypeSocket type=BlockingSocket, const char* theIP=NULL);

  Socket* Accept();
  string address(); 
  unsigned short port();   
};

class SocketSelect 
{
  public:
    SocketSelect(Socket const * const s1, Socket const * const s2=NULL, TypeSocket type=BlockingSocket);

    bool Readable(Socket const * const s);

  private:
    fd_set fds_;
}; 

class SocketException : public Exception
{
private:
	string msg;
public:
	SocketException(const char* m);
	SocketException(string m);
	virtual ~SocketException() {};
	virtual string getMessage() const;
};	

#endif
