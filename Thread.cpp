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
#include "Trace.h"
#include "Thread.h"

#ifdef WIN32
const int Thread::P_ABOVE_NORMAL = THREAD_PRIORITY_ABOVE_NORMAL;
const int Thread::P_BELOW_NORMAL = THREAD_PRIORITY_BELOW_NORMAL;
const int Thread::P_HIGHEST = THREAD_PRIORITY_HIGHEST;
const int Thread::P_IDLE = THREAD_PRIORITY_IDLE;
const int Thread::P_LOWEST = THREAD_PRIORITY_LOWEST;
const int Thread::P_NORMAL = THREAD_PRIORITY_NORMAL;
const int Thread::P_CRITICAL = THREAD_PRIORITY_TIME_CRITICAL;
#define THREAD_NULL NULL
#define ASSIGN_LONG(dest,val) InterlockedExchange(&dest,val)
#define ASSIGN_BOOL(dest,val) InterlockedExchange(&dest,(LONG)val)
#define ASSIGN_PTR(dest,val)  InterlockedExchangePointer(&dest,val)
#else
#include <sys/time.h>
const int Thread::P_ABOVE_NORMAL = 0;
const int Thread::P_BELOW_NORMAL = 1;
const int Thread::P_HIGHEST = 2;
const int Thread::P_IDLE = 3;
const int Thread::P_LOWEST = 4;
const int Thread::P_NORMAL = 5;
const int Thread::P_CRITICAL = 6;
#define THREAD_NULL 0
#define ASSIGN_LONG(dest,val) dest=val
#define ASSIGN_BOOL(dest,val) dest=val
#define ASSIGN_PTR(dest,val)  dest=val
#endif

#define SUSPENDWAITMS 10

bool Thread::itsShutdownInProgress=false;

void Thread::running()
{
	ASSIGN_BOOL(itsRunningFlag,true); 
};

bool Thread::isRunning() 
{
	return (itsRunningFlag!=0) ? true : false; 
};

bool Thread::isSuspended() 
{
	return (itsSuspendedFlag!=0) ? true : false; 
};

void Thread::shutdownInProgress()
{
	TRACE("-----------------------SHUTDOWN-----------------------")
	itsShutdownInProgress=true; 
}

unsigned long Thread::threadID() 
{
#ifdef WIN32
	return GetCurrentThreadId();
#else
	return (unsigned long)pthread_self();
#endif
}

/** Thread(const char* nm)
  * overloaded constructor
  * creates a Thread object identified by "nm"
**/  
Thread::Thread(const char* nm)
{
	TRACE("Thread constructor - start")
	TRACE("Name=" << nm)
	m_hThread = THREAD_NULL;
	m_strName = nm;
	ASSIGN_BOOL(itsRunningFlag,false);
	ASSIGN_BOOL(itsSuspendedFlag,false);
	ASSIGN_LONG(itsWorkingThreadID,0);	

#ifdef WIN32	
	InitializeCriticalSection(&m_hMutex);
#else
	pthread_mutex_init(&m_hMutex,NULL);
#endif

	TRACE("Thread constructor - end")
}

Thread::~Thread() 
{
	TRACE("Thread destructor - start")
	TRACE("Name=" << getName())
	if(m_hThread != THREAD_NULL) 
	{
		stop(true);
	}

#ifdef WIN32
	DeleteCriticalSection(&m_hMutex);	
#else
	pthread_mutex_destroy(&m_hMutex);
#endif

	TRACE("Thread destructor - end")
}


/** getName()
  * return the Thread object's name as a string
**/  
const char* Thread::getName() const 
{	
	return m_strName.c_str();
}

bool Thread::is(const char* theName)
{
	return (m_strName==theName);
}

/** run()
  * called by the thread callback _ou_thread_proc()
  * to be overridden by child classes of Thread
**/ 
void Thread::run() 
{
	// Base run
}

/** sleep(long ms)
  * holds back the thread's execution for
  * "ms" milliseconds
**/ 

void Thread::sleep(long ms) 
{
	TRACE("Thread::sleep(static) - start")
#ifdef WIN32
	Sleep(ms);
#else
    struct timespec abs_ts;
    struct timespec rm_ts;
	rm_ts.tv_sec = ms/1000; 
	rm_ts.tv_nsec = ms%1000 *1000000;

	do
	{
		abs_ts.tv_sec = rm_ts.tv_sec; 
		abs_ts.tv_nsec = rm_ts.tv_nsec;
	} while(nanosleep(&abs_ts,&rm_ts) < 0);
	
#endif
	TRACE("Thread::sleep(static) - stop")
}

/** start()
  * creates a low-level thread object and calls the
  * run() function
**/ 


void Thread::start() 
{
	TRACE("Thread::start - start")
	TRACE("Name=" << getName())
#ifdef WIN32	
	DWORD tid = 0;	
	m_hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)_ou_thread_proc,(Thread*)this,0,&tid);
	if(m_hThread == NULL) 
	{
		TRACE("Fail to create thread")
		throw ThreadException(string("Failed to create thread ->")+m_strName);
	}
	else 
	{
		setPriority(Thread::P_NORMAL);
	}
#else
	pthread_mutex_init(&m_hSuspendMutex,NULL);
	pthread_cond_init(&m_SuspendCondition,NULL);
	int iret = pthread_create( &m_hThread, NULL, _ou_thread_proc,this);
	if(iret!=0)
	{
		TRACE("Fail to create thread")
		throw ThreadException(string("Failed to create thread ->")+m_strName);
	}
#endif
	TRACE("Thread::start - end")
}

/** stop()
  * stops the running thread and frees the thread handle
**/ 
void Thread::stop(bool cancel) 
{
	TRACE("Thread::stop - start")
	TRACE("Name=" << getName())

	if(itsRunningFlag) 
	{
		ASSIGN_BOOL(itsRunningFlag,false);

#ifdef WIN32
		if(isSuspended()==true)
			resume();
			
		if(cancel==true)
			TerminateThread(m_hThread,NULL);
		else
			WaitForSingleObject(m_hThread,INFINITE);
	
		CloseHandle(m_hThread);		
#else
		TRACE("Joining thread")
		if(cancel==true)
			pthread_cancel(m_hThread);
		else
			pthread_join(m_hThread,NULL);

		TRACE("Thread cleanup")			
		pthread_mutex_destroy(&m_hSuspendMutex);
		pthread_cond_destroy(&m_SuspendCondition);
#endif

		m_hThread = THREAD_NULL;
	}

	TRACE("Thread::stop - end")
}

/** setPriority(int tp)
  * sets the priority of the thread to "tp"
  * "tp" must be a valid priority defined in the
  * Thread class
**/ 
void Thread::setPriority(int tp) 
{
	if(m_hThread == THREAD_NULL) 
	{
		throw ThreadException("Thread object is null");
	}
	else 
	{
#ifdef WIN32
		if(SetThreadPriority(m_hThread,tp) == 0) 
		{
			throw ThreadException("Failed to set priority");
		}
#endif
	}
}

void Thread::setAffinity(unsigned cpu)
{
#ifdef WIN32
	DWORD mask=1;
	mask <<= cpu;
	if(SetThreadAffinityMask(m_hThread,mask)==0)
		throw ThreadException("Failed to set affinity");
#else
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
#ifndef P2_PTHREAD_SETAFFINITY
	if(pthread_setaffinity_np(m_hThread, sizeof(cpuset), &cpuset)!=0)
		throw ThreadException("Failed to set affinity");
#else
	if(pthread_setaffinity_np(m_hThread, &cpuset)!=0)
		throw ThreadException("Failed to set affinity");
#endif
#else
	DISPLAY("Thread affinity not supported on this system")
#endif
#endif
	sleep(0); 
}


/** suspend()  
  * suspends the thread
**/ 
void Thread::suspend() 
{
	TRACE("Thread::suspend - start")
	TRACE("Name=" << getName())
	
	if(m_hThread == THREAD_NULL) 
	{
		throw ThreadException(string("Thread object is null ->")+m_strName);
	}
	else 
	{
#ifdef WIN32
		ASSIGN_BOOL(itsSuspendedFlag,true);
		if(SuspendThread(m_hThread) < 0) 
		{
			TRACE("Failed to suspend thread")
			throw ThreadException(string("Failed to suspend thread ->")+m_strName);
		}
#else
		TRACE("Tread suspended")

		pthread_mutex_lock(&m_hSuspendMutex);
		TRACE("Cond mutex count=" << m_hSuspendMutex.__m_count)
		TRACE("Cond lock status=" << m_hSuspendMutex.__m_lock.__status)
		itsSuspendedFlag=true;

		while(itsSuspendedFlag==true)
		{
			int ms=SUSPENDWAITMS;
			struct timespec abs_ts;
		 	struct timeval cur_tv;
			gettimeofday(&cur_tv, NULL);
			abs_ts.tv_sec = cur_tv.tv_sec + ms/1000; 
			abs_ts.tv_nsec = (cur_tv.tv_usec + ms%1000*1000)*1000;

			// FIX by Steve Rodgers
			if(abs_ts.tv_nsec > 999999999)
			{
            	abs_ts.tv_sec++;
                abs_ts.tv_nsec -= 1000000000;
            }
            // End fix
			
			pthread_cond_timedwait(&m_SuspendCondition,&m_hSuspendMutex,&abs_ts);
			TRACE("Cond condition lock status=" << m_SuspendCondition.__c_lock.__status)
			
			if(itsRunningFlag==false)
			{
				TRACE("Resume thread for shutdown cleanup")
				break;
			}
		}
		
		pthread_mutex_unlock(&m_hSuspendMutex);
		TRACE("Cond mutex count=" << m_hSuspendMutex.__m_count)
		TRACE("Cond lock status=" << m_hSuspendMutex.__m_lock.__status)

		TRACE("Thread resumed")
#endif
	}

	TRACE("Thread::suspend - end")	
}

/** resume()  
  * resumes a suspended thread
**/ 
void Thread::resume() 
{
	TRACE("Thread::resume - start")
	TRACE("Name=" << getName())
		
	if(m_hThread == THREAD_NULL) 
	{
		throw ThreadException(string("Thread object is null ->")+m_strName);
	}
	else 
	{
#ifdef WIN32
		if(ResumeThread(m_hThread) < 0) 
		{
			TRACE("Failed to resume thread")
			throw ThreadException(string("Failed to resume thread ->")+m_strName);
		}
		else
		{
			ASSIGN_BOOL(itsSuspendedFlag,false);			
		}
#else
		pthread_mutex_lock(&m_hSuspendMutex);
		TRACE("Cond mutex count=" << m_hSuspendMutex.__m_count)
		TRACE("Cond lock status=" << m_hSuspendMutex.__m_lock.__status)
		itsSuspendedFlag=false;
		pthread_cond_signal(&m_SuspendCondition);
		TRACE("Cond condition lock status=" << m_SuspendCondition.__c_lock.__status)
		pthread_mutex_unlock(&m_hSuspendMutex);
		TRACE("Cond mutex count=" << m_hSuspendMutex.__m_count)
		TRACE("Cond lock status=" << m_hSuspendMutex.__m_lock.__status)
#endif
	}
	TRACE("Thread::resume - end")
}

/** wait(long ms)  
  * makes the thread suspend execution until the
  * mutex is released by another thread.
  * "ms" specifies a time-out for the wait operation.
  * "ms" defaults to 5000 milli-seconds
**/ 
bool Thread::wait(long ms) 
{
	TRACE("Thread::wait - start")		
	TRACE("Name=" << getName())
	
	if(itsWorkingThreadID!=0)
	{
		TRACE(getName() << ": thread " << threadID() << " is waiting for thread " << itsWorkingThreadID) 
	}
	
#ifdef WIN32
	Sleep(0);
	EnterCriticalSection(&m_hMutex);
	ASSIGN_LONG(itsWorkingThreadID,threadID());
#else
	struct timespec abs_ts;
 	struct timeval cur_tv;
	gettimeofday(&cur_tv, NULL);
	abs_ts.tv_sec = cur_tv.tv_sec + ms/1000; 
	abs_ts.tv_nsec = (cur_tv.tv_usec + ms%1000*1000)*1000;
 
	int res=pthread_mutex_timedlock(&m_hMutex,&abs_ts);	
	TRACE("Mutex count=" << m_hMutex.__m_count)
	TRACE("Lock status=" << m_hMutex.__m_lock.__status)

	switch(res) 
	{
		case 0:
			itsWorkingThreadID=threadID(); //++ v1.5
			TRACE("Thread::wait - end")	
			return true;
		case EINVAL:
			throw ThreadException(string("pthread_mutex_timedlock: Invalid value ->")+m_strName);
			break;
		case ETIMEDOUT:
			throw ThreadException(string("pthread_mutex_timedlock: Wait timed out ->")+m_strName);
			break;
		default:
			throw ThreadException(string("pthread_mutex_timedlock: Unexpected value ->")+m_strName);		
	}
#endif
	return false;
}

/** release()  
  * releases the mutex "m" and makes it 
  * available for other threads
**/ 
void Thread::release() 
{
	TRACE("Thread::release - start")	
	TRACE("Name=" << getName())
#ifdef WIN32
	ASSIGN_LONG(itsWorkingThreadID,0L);
	LeaveCriticalSection(&m_hMutex);
#else
	pthread_mutex_unlock(&m_hMutex);	
	TRACE("Mutex count=" << m_hMutex.__m_count)
	TRACE("Lock status=" << m_hMutex.__m_lock.__status)
#endif
	TRACE("Thread::release - end")	
}

// ThreadException
ThreadException::ThreadException(const char* m) 
{
	msg = m;
}

ThreadException::ThreadException(string m) 
{
	msg = m;
}

string ThreadException::getMessage() const 
{
	return msg;
}

// global thread callback
#ifdef WIN32
unsigned int _ou_thread_proc(void* param) 
{
	TRACE("Start _ou_thread_proc")
	try
	{	
		Thread* tp = (Thread*)param;
		tp->running();
		TRACE("Thread " << tp->getName() << "started")
		tp->run();
	}
	catch(...)
	{
		DISPLAY("Unhandled exception in thread callback")
	}

	TRACE("End _ou_thread_proc")	
	return 0;
}
#else
void* _ou_thread_proc(void* param) 
{
	TRACE("Start _ou_thread_proc")	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	try
	{
		Thread* tp = (Thread*)param;
		tp->running();
		TRACE("Thread " << tp->getName() << " started")
		tp->run();
	}
	catch(...)
	{
		DISPLAY("Unhandled exception in thread callback")
	}

	TRACE("End _ou_thread_proc")
	pthread_exit(NULL);
	return NULL;
}
#endif
