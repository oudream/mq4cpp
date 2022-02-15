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

#ifndef __TIMER__
#define __TIMER__

#include "MessageQueue.h"

#ifdef  WIN32
#include <sys/timeb.h>
#define _TIMEVAL timeb
#define _TIMEDIFF timeb
#else
#include <sys/time.h>
#define _TIMEVAL struct timeval
#define _TIMEDIFF struct timeval
#endif

class Wakeup : public Message
{
protected:
	MQHANDLE itsTargetQueue; 
	_TIMEVAL itsTimeVal;
	_TIMEDIFF itsWakeTime;
	bool itsRepeatFlag;

public:	
	Wakeup(Wakeup& o); // ++ v1.5
	Wakeup(MessageQueue* theQueue,long ms,bool repeat=true);
	virtual ~Wakeup();
	virtual Message* clone(); // ++ v1.5
	virtual void toStream(ostream& theStream);
	virtual MQHANDLE getQueue() { return itsTargetQueue; };

#ifdef  WIN32
	virtual unsigned long getTime() { return itsTimeVal.time; };
#else
	virtual unsigned long getTime() { return itsTimeVal.tv_sec; };
#endif

	virtual bool repeat() { return itsRepeatFlag; };
	virtual void setTime();
	virtual bool isExpired();
};

class Timer : public Thread, public LinkedList
{
protected:
	static Timer* itsDefaultTimer;
	Timer();
	void run();
	bool onIteration(LinkedElement* theMessage);
	virtual void onException(Exception& ex);
	virtual void deleteObject(void* theObject) { delete (Wakeup*)theObject; }; //++ v1.5

public:
	Timer(const char* theTimerName);
	virtual ~Timer();
	virtual void schedule(Wakeup* theWakeup);

	static void postToDefaultTimer(Wakeup* theWakeup);
	static void waitForCompletion();
	static unsigned long time();
	static _TIMEVAL timeExt();
	static long subtractMillisecs(_TIMEVAL* x,_TIMEVAL* y);
};

#define SCHEDULE(queue,time) \
	Timer::postToDefaultTimer(new Wakeup(queue,time));
#define STOPTIMER() \
	Timer::waitForCompletion();
#endif

#define TIME_POINT \
	_TIMEVAL Macro_StartTime=Timer::timeExt();

#define TRACE_ELAPSED \
	{ _TIMEVAL Macro_EndTime=Timer::timeExt(); \
	  TRACE("Elapsed " << Timer::subtractMillisecs(&Macro_StartTime,&Macro_EndTime) << " ms") \
	  Macro_StartTime=Macro_EndTime; }

#define DISPLAY_ELAPSED \
	{ _TIMEVAL Macro_EndTime=Timer::timeExt(); \
	  DISPLAY("Elapsed " << Timer::subtractMillisecs(&Macro_StartTime,&Macro_EndTime) << " ms") \
	  Macro_StartTime=Macro_EndTime; }
	
