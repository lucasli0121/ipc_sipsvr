#include "pch.h"
#include"sip_log.h"
#include <time.h>

static SipLog* _theLog = NULL;

static void app_log_writer(int level, const char *buffer, int len)
{
	if (_theLog) {
		_theLog->writeLog(level, buffer, len);
	}
}


SipLog::SipLog()
{
	_file = NULL;
	_theLog = this;
		
}
SipLog::~SipLog()
{
	unini();
}

std::string SipLog::createLogName()
{
	char val[255] ;
	time_t t;
	time(&t);
	struct tm* ptm = localtime(&t);
	sprintf(val,
		"../log/sip_log_%4d%02d%02d%02d%02d%02d.log",
		ptm->tm_year + 1900,
		ptm->tm_mon + 1,
		ptm->tm_mday,
		ptm->tm_hour,
		ptm->tm_min,
		ptm->tm_sec);
	return std::string(val);
}
/*
 * open log file
*/
void SipLog::init()
{
	int log_level = 5;
	std::string fileName = createLogName();
	_fileTotalSize = ConfigMgr::ins()->getLogFileSize();
	if (!_file) {
		_file = fopen(fileName.c_str(), "w+");
		pj_log_set_log_func(&app_log_writer);
		pj_log_set_level(log_level);
	}
}

void SipLog::unini()
{
	if (_file) {
		fclose(_file);
		_file = NULL;
	}
}

void SipLog::writeLog(int level, const char *buffer, int len)
{
	/* Write to both stdout and file. */
	pj_log_write(level, buffer, len);

	if (_file) {
		pj_size_t count = fwrite(buffer, len, 1, _file);
		PJ_UNUSED_ARG(count);
		fflush(_file);
		long flen = ftell(_file);
		if (flen >= _fileTotalSize) {
			fclose(_file);
			std::string name = createLogName();
			if (name.length() > 0) {
				_file = fopen(name.c_str(), "w+");
			}
		}
	}
}

void SipLog::errorLog(const char *sender, const char *title, pj_status_t status)
{
	char errmsg[PJ_ERR_MSG_SIZE];

	pj_strerror(status, errmsg, sizeof(errmsg));
	SIP_LOG(SIP_ERROR, (sender, "%s: %s [status=%d]", title, errmsg, status));
}