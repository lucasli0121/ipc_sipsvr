#pragma once

#include <stdio.h>
#include "pjproject.h"
/*
 * for log
*/

#define SIP_FATAL   0
#define SIP_ERROR   1
#define SIP_WARNING 2  
#define SIP_INFO    3
#define SIP_DEBUG   4
#define SIP_TRACE   5

#ifndef SIP_LOG
#define SIP_LOG(level, arg) PJ_LOG(level, arg)
#endif

#ifndef ERROR_LOG
#define ERROR_LOG(send, title, status) SipLog::errorLog(send, title, status)
#endif

class TraceFunc
{
public:
	TraceFunc(const char* name)
	{
		_name = name;
		SIP_LOG(SIP_DEBUG, ("enter function: %s", name));
	}
	~TraceFunc() {
		SIP_LOG(SIP_DEBUG, ("leave function: %s", _name));
	}
private:
	const char* _name;
};

#ifdef _DEBUG
#define TRACE_FUNC(x) TraceFunc __traceFunc(x)
#else
#define TRACE_FUNC(x)
#endif

class LockGuard
{
public:
	LockGuard(pj_lock_t* lock) {
		_lock = lock;
		if (_lock) {
			pj_lock_acquire(_lock);
		}
	}
	~LockGuard() {
		if (_lock) {
			pj_lock_release(_lock);
		}
	}
private:
	pj_lock_t * _lock;
};
class SipLog
{
public:
	SipLog();
	~SipLog();
	void init();
	void unini();
	static void errorLog(const char *sender, const char *title, pj_status_t status);
	void writeLog(int level, const char *buffer, int len);
private:
	std::string createLogName();
private:
	FILE * _file;
	int _fileTotalSize;
};