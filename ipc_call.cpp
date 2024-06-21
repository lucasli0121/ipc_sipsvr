/*
* IpcCall.cpp ��
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h"   // NOLINT
#include "ps_codec.h" // NOLINT
#include "stream_mgr.h" // NOLINT

static const char* THIS_FILE = "IpcCall";
static IpcCall* _theCall = NULL;


IpcCall::IpcCall(const char* deviceId, const char* ipcAddr, int ipcPort, int localRtpPort, uint32_t index)
{
	_deviceId = deviceId;
	_ipcPort = ipcPort;
	_localRtpPort = localRtpPort;
	_theCall = this;
	_hasStart = false;
	memset(&_call, 0, sizeof(_call));
	_call.index = index;
	if (ipcAddr != NULL) {
		_ipcAddr = ipcAddr;
		StreamMgr::ins()->addIpcChannel(ipcAddr, ipcPort);
	}
	_frame = ConfigMgr::ins()->getFrame();
	_remoteRtpPort = 0;
	_fmt = "ps";
	_pool = NULL;
	pj_gettimeofday(&_activeTm);
}

std::string IpcCall::getRecvAddr()
{
	char val[64];
	memset(val, 0, sizeof(val));
	std::string recvIp = ConfigMgr::ins()->getRtpRecvIp();
	if (recvIp == "0.0.0.0" || recvIp.empty()) {
		/*
		* Generate Contact URI
		*/
		pj_sockaddr hostaddr;
		pj_status_t status;
		if ((status = pj_gethostip(pj_AF_INET(), &hostaddr)) != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "Unable to retrieve local host IP", status);
		} else {
			pj_sockaddr_print(&hostaddr, val, sizeof(val), 2);
		}
		recvIp = val;
	}
	return recvIp;
}

void IpcCall::setSdpAttr(int frame, int remotePort, const char* fmt)
{
	_remoteRtpPort = remotePort;
	_frame = frame;
	if (fmt) {
		_fmt = fmt;
	}
}

void IpcCall::updateActivateTm()
{
	pj_gettimeofday(&_activeTm);
}
long IpcCall::getInActivateTmLen()
{
	pj_time_val ts;
	pj_gettimeofday(&ts);
	PJ_TIME_VAL_SUB(ts, _activeTm);
	long msec = PJ_TIME_VAL_MSEC(ts);
	return msec;
}
