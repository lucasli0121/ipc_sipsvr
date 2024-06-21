/*
* config_mgr.cpp ��
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h"  // NOLINT
#include "config_mgr.h" // NOLINT

ConfigMgr* ConfigMgr::_mgr = NULL;
static const char* THIS_FILE = "config_mgr.cpp";

ConfigMgr::ConfigMgr()
{
	_fileBufSize = 1024*1024;
	_logFileSize = 1024 * 1024;
	_sipHost = "127.0.0.1";
	_sipPort = 5060;
	_sipId = "34020000002000000001";
	_channelId = "34020000001310000001";
	_startRtpPort = 4000;
	_rtpPortStep = 2;
	_videoFormat = "PS/96,H264/97";
	_frame = 15;
	_bitRate = 1500;
	_clockRate = 90000;
	_mqServerAddr = std::string("127.0.0.1");
	_mqPort = 1883;
	_pubQos = 2;
	_subQos = 2;
	_msgCount = 10;
	_msgSize = 1024;
	_forceKeyFrame = 1;
	_timeout = 120;
	ConfigMgr::_mgr = this;
}

ConfigMgr::~ConfigMgr()
{
	_mqServerAddr.clear();
}

void ConfigMgr::setMqServerAddr(const char* addr, int port)
{
	if (addr) {
		_mqServerAddr = addr;
	}
	_mqPort = port;
}
void ConfigMgr::setMqParams(int subQos, int pubQos, int msgCount, int msgSize)
{
	_subQos = subQos;
	_pubQos = pubQos;
	_msgCount = msgCount;
	_msgSize = msgSize;
}

int ConfigMgr::getVideoCodec(std::vector<codec> &vec)
{
	try {
		std::vector<std::string> vecStr;
		boost::split(vecStr, _videoFormat, boost::is_any_of(","), boost::token_compress_on);
		for (std::vector<std::string>::iterator it = vecStr.begin(); it != vecStr.end(); ++it) {
			MyRegEx reg("(.+)\\/([0-9]+)");
			MyMatch m;
			codec obj;
			obj.frame = _frame;
			obj.bit_rate = _bitRate;
			obj.clock_rate = _clockRate;
			strcpy(obj.desc, it->c_str());
			if (MyRegSearch(it->c_str(), m, reg) && m.size() >= 3) {
				obj.pt = atoi(m[2].str().c_str());
				strcpy(obj.name, m[1].str().c_str());
				vec.push_back(obj);
			}
		}
		
	} catch (MyRegError &ex) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "regex,err:%d, %s\n", ex.code(), ex.what()));
		return -1;
	} catch (boost::wrapexcept<boost::bad_function_call>& ex) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "regex,err: %s\n", ex.what()));
		return -1;
	} catch (std::exception& ex) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "regex,err: %s\n", ex.what()));
		return -1;
	}
	return 0;
}
