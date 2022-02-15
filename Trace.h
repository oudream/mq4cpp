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
// For WinDbg users:
// -----------------
// There's a problem with OutputDebugString / TRACE statements running under
// the debugger near synchronization conditions. 
// The problem is documented roughly in the article 'PRB: Synchronization
// Failure When Debugging' on MSDN. As I understand it, it's more of a
// limitation of the Win32 debug environment rather than any specific debugger.
// The solution unfortunately is to remove the TRACE / OutputDebugString calls
// from around the code making SetEvent / WaitForSingleObject (or other
// synchronization routines). This problem doesn't occur if you don't use WinDbg 
// because TRACE macro switch debug messages to stdout. This work fine on IA32 if you
// redirect output to a file. Redirection doesn't work well on W2003/IA64 because writing
// on a file isn't thread safe. This problem occur expecially with Thread.cpp invoking wait(), 
// release() or suspend().
//

#ifndef __TRACE__
#define __TRACE__

#include <iostream>
#include <strstream>
using namespace std;
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#define DISPLAY(a) \
	cout << __FILE__ << "(" << __LINE__ << ")" << ": " << a << endl;

#define BREAKPOINT \
		DebugBreak();

#ifndef SILENT
extern void trace_dump(const char* descr,char* theString,int theLen);
#define DUMP(a,b,c) trace_dump(a,b,c)
#ifdef WIN32
class DebugStream : public ostream
{
 private:
	class DebugStreamBuf : public std::strstreambuf
	{
	protected:
		virtual int sync()
		{
			sputc('\0');
			Sleep(0); // MS workaround, but you can continue to have strange problems...
			OutputDebugString(str());
			Sleep(0);
		
			freeze(false);
			setp(pbase(), pbase(), epptr());
			return 0;
		}
	};

	DebugStreamBuf m_buf;

	public:
	DebugStream() : ostream(&m_buf) {};
	~DebugStream() { m_buf.pubsync();};
};

#define TRACE(a) \
	if(IsDebuggerPresent()) \
	{ \
		DebugStream dout; \
		dout << __FILE__ << "(" << __LINE__ << ") #" << GetCurrentThreadId() << ": " << a << endl << ends; \
	} \
	else \
	{ \
		cout << __FILE__ << "(" << __LINE__ << ") #" << GetCurrentThreadId() << ": " << a << endl; \
	}
#else
#define TRACE(a) \
	cout << __FILE__ << "(" << __LINE__ << ") #" << (int)pthread_self() << ": " << a << endl;
#endif
#else
#define TRACE(a) ;
#define DUMP(a,b,c)
#endif

#endif

