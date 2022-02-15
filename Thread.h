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


#ifndef __THREAD__
#define __THREAD__

#include "Exception.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/errno.h>
#endif

class ThreadException : public Exception
{
private:
	string msg;
public:
	ThreadException(string m);
	ThreadException(const char* m);
	virtual ~ThreadException() {};
	virtual string getMessage() const;
};	

class Thread 
{
protected:
		string m_strName;

#ifdef WIN32	
		HANDLE m_hThread;
		CRITICAL_SECTION m_hMutex;
		LONG volatile itsWorkingThreadID;
		LONG volatile itsRunningFlag;
		LONG volatile itsSuspendedFlag;
#else
		pthread_t m_hThread;
		pthread_mutex_t m_hMutex;
		pthread_mutex_t m_hSuspendMutex;
		pthread_cond_t m_SuspendCondition;
		unsigned long itsWorkingThreadID;
		bool itsRunningFlag;
		bool itsSuspendedFlag;
#endif

		static bool itsShutdownInProgress;

public:
		Thread(const char* nm);
		virtual ~Thread();
		const char* getName() const;
		void start();
		virtual void run();
		static void sleep(long ms);
		void suspend();
		void resume();
		void stop(bool cancel=true);

		void setPriority(int p);
		void setAffinity(unsigned cpu);

		bool wait(long ms=5000);
		void release();
		
		bool is(const char* theName);
		void running();
		bool isRunning();
		bool isSuspended();
		
		static void shutdownInProgress();
		static bool isShuttingDown() { return itsShutdownInProgress; };
		static unsigned long threadID();		

public:
		// Thread priorities
		static const int P_ABOVE_NORMAL;
		static const int P_BELOW_NORMAL;
		static const int P_HIGHEST;
		static const int P_IDLE;
		static const int P_LOWEST;
		static const int P_NORMAL;
		static const int P_CRITICAL;
				
};

extern "C" 
{
#ifdef WIN32	
	unsigned int _ou_thread_proc(void* param);
#else
	void* _ou_thread_proc(void* param);
#endif
}

#endif
				
