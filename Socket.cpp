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
// Original Win32 code by Vijay R. Nyffenegger
// Modified to support Linux socket by Riccardo Pompeo
// Linux accept()/pthread_cancel problem resolved by Duncan McDiarmid
// Linux info about network interfaces by Floyd Davidson 
// 

#define SILENT
#include "Socket.h"
#include "Trace.h"
#ifndef WIN32
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define TIMEVAL struct timeval
#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))
#define MAC_ADDRESS_CHAR_LEN  6
#else
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <errno.h>
#endif

int Socket::nofSockets_= 0;

vector<NetAdapter>* Socket::getAdapters()
{
  TRACE("Socket::getAdapters - start")
  vector<NetAdapter>* ret=new vector<NetAdapter>;

#ifdef WIN32
  PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
  ULONG OutBufferLength = 0;
  ULONG RetVal = 0;    
    
  // Realloc buffer size until no overflow occurs
  do 
  {
      RetVal = GetAdaptersAddresses(AF_INET,0,NULL,AdapterAddresses,&OutBufferLength);
      if (RetVal == ERROR_BUFFER_OVERFLOW) 
      {
        if (AdapterAddresses != NULL) 
          delete(AdapterAddresses);
        
        AdapterAddresses =(PIP_ADAPTER_ADDRESSES) new char[OutBufferLength];
      }
  }  while (RetVal == ERROR_BUFFER_OVERFLOW); 

  if (RetVal == NO_ERROR) 
  {
      // If successful, output some information from the data we received
      PIP_ADAPTER_ADDRESSES AdapterList = AdapterAddresses;
      while (AdapterList) 
      {
         PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddress;
      	 char szAdapterName[64];
      	
         int len = WideCharToMultiByte(CP_ACP, 0, AdapterList->FriendlyName, wcslen(AdapterList->FriendlyName),
                                       szAdapterName, sizeof(szAdapterName),NULL, NULL);
		 if (len == 0)
		 {
           delete(AdapterAddresses);
	       throw SocketException("Cannot convert adapter name");
		 }
		 
	     szAdapterName[len] = '\0';

         for (pUnicastAddress = AdapterList->FirstUnicastAddress; pUnicastAddress; pUnicastAddress = pUnicastAddress->Next)
         {
	       char szAddress[NI_MAXHOST];

	       if (getnameinfo(pUnicastAddress->Address.lpSockaddr, pUnicastAddress->Address.iSockaddrLength,
			               szAddress, sizeof(szAddress), NULL, 0, NI_NUMERICHOST))
		   {
             delete(AdapterAddresses);
		     throw SocketException("Can't convert network format to presentation format");
		   }

           TRACE("Adapter name=" << szAdapterName)
           TRACE("Adapter address=" << szAddress)
           DUMP("Adapter phisical address",(char*)AdapterList->PhysicalAddress,AdapterList->PhysicalAddressLength);

		   NetAdapter anAdapter(string(szAdapterName), string(szAddress),
		                        string((char*)AdapterList->PhysicalAddress,AdapterList->PhysicalAddressLength));
		   ret->push_back(anAdapter);
         }

         AdapterList = AdapterList->Next;
      }
  }
  else 
  { 
      LPVOID lpMsgBuf;
      if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, RetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,0,NULL)) 
      {
         string msg=(char*)lpMsgBuf;
         LocalFree(lpMsgBuf);
         delete(AdapterAddresses);
         throw SocketException(msg);
      }
      LocalFree( lpMsgBuf );
  }

  delete(AdapterAddresses);

#else
  int  sockfd;
  int  size = 1;
  ifreq*  ifr;
  ifconf  ifc;
  sockaddr_in sa;
  string  cur_interface_name;

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sockfd ==-1)
    throw SocketException("Cannot open socket");

  ifc.ifc_len = IFRSIZE;
  ifc.ifc_req = NULL;

  do
  {
    ++size;

    ifc.ifc_req = static_cast<ifreq*>(realloc(ifc.ifc_req, IFRSIZE));
    if (ifc.ifc_req == NULL)
      throw SocketException("Out of memory");

    ifc.ifc_len = IFRSIZE;
    if (ioctl(sockfd, SIOCGIFCONF, &ifc))
      throw SocketException("Error ioctl SIOCFIFCONF");

  } while  (IFRSIZE <= ifc.ifc_len);

  ifr = ifc.ifc_req;
  for(; (ifr - ifc.ifc_req) * sizeof(ifreq) < static_cast<unsigned long>(ifc.ifc_len); ++ifr)
  {
    if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data) continue;  // duplicate, skip it

    if (ioctl(sockfd, SIOCGIFFLAGS, ifr) != 0) continue;  // failed to get flags, skip it

    string name = ifr->ifr_name;

    // ------ get IP_ADDRESS ------
    string ip = inet_ntoa(inaddrr(ifr_addr.sa_data));

    // ------ get HW_ADDRESS ------
#ifdef __FreeBSD__
    if (ioctl(sockfd, SIOCGIFMAC, ifr) != 0) continue;  // failed to get mac, skip it
#else
    if (ioctl(sockfd, SIOCGIFHWADDR, ifr) != 0) continue;  // failed to get mac, skip it
#endif

    string mac=string((char*)ifr->ifr_addr.sa_data,MAC_ADDRESS_CHAR_LEN);

    TRACE("Adapter name=" << name)
    TRACE("Adapter address=" << ip)
    DUMP("Adapter phisical address",(char*)ifr->ifr_addr.sa_data,MAC_ADDRESS_CHAR_LEN)
    NetAdapter anAdapter(name, ip, mac);
    ret->push_back(anAdapter);
  }

  close(sockfd);
  free(ifc.ifc_req);

#endif	

  TRACE("Socket::getAdapters - end")
  return ret;	
}

void Socket::Start() 
{
  TRACE("Socket::Start - start")
  if (!nofSockets_) 
  {
#ifdef WIN32  	
    WSADATA info;
    if (WSAStartup(MAKEWORD(2,0), &info)) 
      	throw SocketException("Socket: Could not start WSA");
#endif
  }
  ++nofSockets_;
  TRACE("Socket::Start - end")
}

void Socket::End() 
{
  TRACE("Socket::End - start")
#ifdef WIN32  	
  WSACleanup();
#endif
  TRACE("Socket::End - end")
}

Socket::Socket() : s_(0) 
{
  TRACE("Socket::Socket - start")
  Start();
  // UDP: use SOCK_DGRAM instead of SOCK_STREAM
  s_ = socket(AF_INET,SOCK_STREAM,0);
#ifdef WIN32
  if (s_ == INVALID_SOCKET)
    throw SocketException("Socket: socket returns error");
#else
  if (s_ < 0) 
  {
  	TRACE("socket return=" << s_)
    throw SocketException("Socket: socket returns error");
  }
#endif
  refCounter_ = new int(1);
  TRACE("Socket::Socket - end")
}

Socket::Socket(SOCKET s) : s_(s) 
{
  TRACE("Socket::Socket - start")
  Start();
  refCounter_ = new int(1);
  TRACE("Socket::Socket - end")
};

Socket::~Socket() 
{
  TRACE("Socket::~Socket - start")
  if (! --(*refCounter_)) 
  {
    Close();
    delete refCounter_;
  }

  --nofSockets_;
  if (!nofSockets_) End();
  TRACE("Socket::~Socket - end")
}

Socket::Socket(const Socket& o) 
{
  TRACE("Socket::Socket - start")
  refCounter_=o.refCounter_;
  (*refCounter_)++;
  s_         =o.s_;

  nofSockets_++;
  TRACE("Socket::Socket - end")
}

Socket& Socket::operator =(Socket& o) 
{
  TRACE("Socket::operator= - start")
  (*o.refCounter_)++;

  refCounter_=o.refCounter_;
  s_         =o.s_;

  nofSockets_++;

  TRACE("Socket::operator= - end")
  return *this;
}

void Socket::Close() 
{
  TRACE("Socket::Close - start")
  if(s_>=0)
  {
#ifdef WIN32  
  	closesocket(s_);
#else
  	shutdown(s_,SHUT_RDWR);
#endif
  }
  s_=-1;
  TRACE("Socket::Close - end")
}

bool Socket::ReceiveBuffer(void* theBuffer,int theLen)
{
  TRACE("Socket::ReceiveBuffer - start")
  TRACE("Buffer len=" << theLen)

  int rxcnt=0;
  while(rxcnt < theLen)
  {
	  int len=theLen-rxcnt;
	  int rv = recv (s_, ((char*)theBuffer)+rxcnt, len, 0);
	  if (rv <= 0)
	  {
	  	 TRACE("recv returns error=" << rv)
		 return  false;
	  }
	  rxcnt+=rv;
  }
  
  TRACE("Socket::ReceiveBuffer - end")
  return true;	 
}

std::string Socket::ReceiveBytes() 
{
  TRACE("Socket::ReceiveBytes - start")
  std::string ret;

  char buf[1024];
  for ( ; ; ) 
  {
	  u_long arg = 1024;

#ifdef WIN32
	  if (ioctlsocket(s_, FIONREAD, &arg) != 0)
	   break;
	  if (arg == 0)
	   break;
	  if (arg > 1024)
	   arg = 1024;
#endif

	  int rv = recv (s_, buf, arg, 0);
	  if (rv <= 0)
	   break;
	  std::string t;
	  t.assign (buf, rv);
	  ret += t;
  }
 
  TRACE("Socket::ReceiveBytes - end")
  return ret;
}

std::string Socket::ReceiveLine() 
{
  TRACE("Socket::ReceiveLine - start")
  std::string ret;
   while (1) 
   {
     char r;

     switch(recv(s_, &r, 1, 0)) 
     {
       case 0: // not connected anymore;
         return "";

       case -1:
#ifdef WIN32       
          if (errno == EAGAIN) 
             return ret;
          else 
             return "";
#else
		  return "";      	  
#endif
     }

     ret += r;
     if (r == '\n')  return ret;
   }
  TRACE("Socket::ReceiveLine - end")
}

void Socket::SendLine(std::string s) 
{
  TRACE("Socket::SendLine - start")
  s += '\n';
  send(s_,s.c_str(),s.length(),0);
  TRACE("Socket::SendLine - end")
}

void Socket::SendBytes(const std::string& s) 
{
  TRACE("Socket::SendBytes - start")
  send(s_,s.data(),s.length(),0);
  TRACE("Socket::SendBytes - end")
}

void Socket::SendBuffer(void* theBuffer,int theLen) 
{
  TRACE("Socket::SendBuffer - start")
  send(s_,(char*)theBuffer,theLen,0);
  TRACE("Socket::SendBuffer - end")
}

SocketServer::SocketServer(int port, int connections, TypeSocket type, const char* theIP) 
{
  TRACE("SocketServer::SocketServer - start")

  sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = PF_INET;             
  sa.sin_port = htons(port);
  TRACE("sa.sin_port" << sa.sin_port)          

  if(theIP!=NULL)
  {
#ifdef WIN32
    sa.sin_addr.S_un.S_addr=inet_addr(theIP);
#else
	inet_aton(theIP,&sa.sin_addr);
#endif
  }

  s_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef WIN32
  if (s_ == INVALID_SOCKET) 
    throw SocketException("SocketServer: socket returns error");

  if(type==NonBlockingSocket) 
  {
    u_long arg = 1;
    ioctlsocket(s_, FIONBIO, &arg);
  }
#else
  if (s_ < 0)
  {
  	TRACE("socket error=" << errno)
    throw SocketException("SocketServer: socket returns error");
  }
#endif

  TRACE("Socket=" << s_)

  /* bind the socket to the internet address */
  int retbind=bind(s_, (sockaddr *)&sa, sizeof(sockaddr_in));
#ifdef WIN32
  if (retbind == SOCKET_ERROR) 
  {
    closesocket(s_);
    throw SocketException("SocketServer: bind returns error");
  }
#else
  if (retbind < 0) 
  {
	TRACE("bind return error=" << errno)
	shutdown(s_,SHUT_RDWR);
    throw SocketException("SocketServer: bind returns error");
  }
#endif
  
  listen(s_, connections);                               
  TRACE("SocketServer::SocketServer - end")
}

Socket* SocketServer::Accept() 
{
  TRACE("SocketServer::Accept - start")
  SOCKET new_sock;
    
#ifdef WIN32 
  int len = sizeof(struct sockaddr); 	
  new_sock = accept(s_, &itsSocketAddr, &len);
  if (new_sock == INVALID_SOCKET) 
  {
     int rc = WSAGetLastError();
     if(rc==WSAEWOULDBLOCK) 
        return 0; // non-blocking call, no request pending
     else 
        throw SocketException("SocketServer: accept returns error");
  }
#else
  socklen_t len = sizeof(struct sockaddr);  
  fd_set read_set;
  FD_ZERO(&read_set);
  TIMEVAL timeout;
  
  for(;;)
  {
	  pthread_testcancel();

	  timeout.tv_sec  = 0;
	  timeout.tv_usec = 1000;
	  FD_SET(s_,&read_set);

	  if(s_<0)
	    throw SocketException("SocketServer: shutdown in progress");

	  int selret=select(s_+1, &read_set, NULL, NULL, &timeout);
	  if(selret<0)
	    throw SocketException("SocketServer: select returns error");
	  else if(selret==0) //detect time-out and check thread cancel
	  	continue;
	  	
	  if(s_<0)
	    throw SocketException("SocketServer: shutdown in progress");
	
	  if(FD_ISSET(s_, &read_set))
	  {
	     new_sock = accept(s_, &itsSocketAddr, &len);
	     break;
	  } 
  }


  if(new_sock <  0)
  {
     TRACE("accept error=" << new_sock)
     throw SocketException("SocketServer: accept returns error");
  }
#endif          

  Socket* r = new Socket(new_sock);
  TRACE("SocketServer::Accept - end")
  return r;
}

string SocketServer::address() 
{
  return inet_ntoa(((struct sockaddr_in*)&itsSocketAddr)->sin_addr);
}

unsigned short SocketServer::port()
{
  TRACE("SocketServer::port - start")
  unsigned short ret=(itsSocketAddr.sa_data[0]<<8)+itsSocketAddr.sa_data[1];
  TRACE("SocketServer::port - end")
  return ret;
}

SocketClient::SocketClient(const std::string& host, int port) : Socket() 
{
  TRACE("SocketClient::SocketClient - start")
  std::string error;

  hostent *he;
  if ((he = gethostbyname(host.c_str())) == 0) 
  {
#ifdef WIN32  	
    error = strerror(errno);
#else
	error = "SocketClient: gethostbyname returns error";
#endif
	TRACE("gethostbyname returns error")
    throw SocketException(error);
  }

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *((in_addr *)he->h_addr);
  memset(&(addr.sin_zero), 0, 8); 

  if (::connect(s_, (sockaddr *) &addr, sizeof(sockaddr))) 
  {
#ifdef WIN32 
    error = strerror(WSAGetLastError());
#else
	error = "SocketClient: connect returns error";
#endif    
	TRACE("connect return error")
    throw SocketException(error);
  }
  TRACE("SocketClient::SocketClient - end")
}

SocketSelect::SocketSelect(Socket const * const s1, Socket const * const s2, TypeSocket type) 
{
  FD_ZERO(&fds_);
  FD_SET(const_cast<Socket*>(s1)->s_,&fds_);
  if(s2) 
  {
    FD_SET(const_cast<Socket*>(s2)->s_,&fds_);
  }     

  TIMEVAL tval;
  tval.tv_sec  = 0;
  tval.tv_usec = 1;

  TIMEVAL *ptval;
  if(type==NonBlockingSocket) 
  {
    ptval = &tval;
  }
  else 
  { 
    ptval = 0;
  }

#ifdef WIN32
  if (select (0, &fds_, (fd_set*) 0, (fd_set*) 0, ptval) == SOCKET_ERROR) 
  	throw SocketException("SocketSelect: select returns error");
#else
  if (select (0, &fds_, (fd_set*) 0, (fd_set*) 0, ptval) < 0) 
  	throw SocketException("SocketSelect: select returns error");
#endif
}

bool SocketSelect::Readable(Socket const * const s) 
{
  if (FD_ISSET(s->s_,&fds_)) return true;
  return false;
}

SocketException::SocketException(const char* m) 
{
	msg = m;
}

SocketException::SocketException(string m) 
{
	msg = m;
}

string SocketException::getMessage() const 
{
	return msg;
}

