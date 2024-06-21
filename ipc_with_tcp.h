#pragma once

#include "ipc_call.h"
#include "TcpTransmitter.h"

class IpcWithTcp : public IpcCall
{
public:
	IpcWithTcp(const char* deviceId, const char* ipcAddr, int ipcPort, int localRtpPort, uint32_t index);
	virtual int start(pj_caching_pool* cp);
	virtual void stop();
	virtual void onMediaUpdate(pjsip_inv_session *inv, pj_status_t status);
	virtual ~IpcWithTcp();
	int acceptThread();
	int receiveThread();
	void onSendError(SocketType sock);
	void onReceiveError(SocketType sock);
private:
	RTPSession * _rtpSession;
	TcpTransmitter _tcpTransmitter;
	pj_pool_t *_pool;
	pj_thread_t *_thread;
	pj_thread_t *_threadRecv;
	SocketType _listenSock;
	std::vector<SocketType> _vecSocks;
};