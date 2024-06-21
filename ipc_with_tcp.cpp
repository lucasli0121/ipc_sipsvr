/*
* ipc_with_tcp.cpp ¡£
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h" // NOLINT
#include "ps_codec.h" // NOLINT
#include "stream_mgr.h" // NOLINT
#include "rtperrors.h" // NOLINT
#include "rtpsocketutil.h" // NOLINT
#include "rtpsocketutilinternal.h" // NOLINT
#include "rtpipv4address.h" // NOLINT
#include "rtpsessionparams.h" // NOLI
#include "rtplibraryversion.h" // NOLINT
#include "rtppacket.h" // NOLINT
#include "rtptcpaddress.h" // NOLINT
#include "TcpRtpSession.h"

#include "ipc_with_tcp.h" // NOLINT

static const char* THIS_FILE = "IpcWithTcp";
using namespace jrtplib;
#define RTP_PACK_SIZE (1388) // RTP header 12bytes,RTP pkg-max-len <= 1388( 2 bytes for H264 Nalu header)

static void onSendErrorFunc(SocketType sock, void* arg)
{
	IpcWithTcp * obj = reinterpret_cast<IpcWithTcp*>(arg);
	if (obj) {
		obj->onSendError(sock);
	}
}
static void onReceiveErrorFunc(SocketType sock, void* arg)
{
	IpcWithTcp * obj = reinterpret_cast<IpcWithTcp*>(arg);
	if (obj) {
		obj->onReceiveError(sock);
	}
}

static int accept_thread_func(void *arg)
{
	IpcWithTcp* obj = reinterpret_cast<IpcWithTcp*>(arg);
	if (obj) {
		return obj->acceptThread();
	}
	return 0;
}
static int receive_rtp_thread(void* arg)
{
	IpcWithTcp* obj = reinterpret_cast<IpcWithTcp*>(arg);
	if (obj) {
		return obj->receiveThread();
	}
	return 0;
}

IpcWithTcp::IpcWithTcp(const char* deviceId, const char* ipcAddr, int ipcPort, int localRtpPort, uint32_t index)
	: IpcCall(deviceId, ipcAddr, ipcPort, localRtpPort, index), _tcpTransmitter("IpcWithTcp")
{
	_pool = NULL;
	_thread = NULL;
	_threadRecv = NULL;
	_listenSock = RTPSOCKERR;
	_rtpSession = NULL;
}
IpcWithTcp::~IpcWithTcp()
{

}

/*
 * start RTP over TCP
 * cp: 
*/
int IpcWithTcp::start(pj_caching_pool* cp)
{
	TRACE_FUNC("IpcWithTcp::start()");

	if (_hasStart) {
		return 0;
	}
	// Create a listener socket and listen on it
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == RTPSOCKERR) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "Can't create listener socket in IpcWithTcp class"));
		return -1;
	}
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(_localRtpPort);
	/*bind the RTP port*/
	if (bind(_listenSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "Can't bind listener socket, port=%d", _localRtpPort));
		RTPCLOSE(_listenSock);
		_listenSock = RTPSOCKERR;
		return -1;
	}
	listen(_listenSock, 1);
	int status = 0;
	int clockRate = ConfigMgr::ins()->getClockRate();
	RTPSessionParams sessparams;
	bool threadsafe = false;
#ifdef RTP_SUPPORT_THREAD
	threadsafe = true;
#endif // RTP_SUPPORT_THREAD
	_tcpTransmitter.Init(threadsafe);
	_tcpTransmitter.Create(65535, 0);
	_tcpTransmitter.setCallbackFunc(onSendErrorFunc, onReceiveErrorFunc, this);
	sessparams.SetProbationType(RTPSources::NoProbation);
	sessparams.SetOwnTimestampUnit(1.0 / (double)clockRate);
	sessparams.SetAcceptOwnPackets(true);
	sessparams.SetUsePredefinedSSRC(true);
	sessparams.SetMaximumPacketSize(RTP_PACK_SIZE + 12);
	sessparams.SetPredefinedSSRC(100);
	TcpRtpSession * sess = new TcpRtpSession();
	_rtpSession = sess;
	status = _rtpSession->Create(sessparams, &_tcpTransmitter);
	if (status < 0) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "rtp session create error:%s", RTPGetErrorString(status).c_str()));
		delete sess;
		RTPCLOSE(_listenSock);
		_listenSock = RTPSOCKERR;
		return -1;
	}
	int tmInc = 1000 / _frame;  // _frame defined in base class
	_rtpSession->SetDefaultPayloadType(96); // H264
	_rtpSession->SetDefaultMark(true);
	_rtpSession->SetDefaultTimestampIncrement(tmInc);
	_pool = pj_pool_create(&cp->factory, "IpcWithTcp", 512, 512, NULL);
	_hasStart = true;
	status = pj_thread_create(_pool, "AcceptThread", accept_thread_func, this, 0, 0, &_thread);
	if (status != PJ_SUCCESS) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "create accept socket thread failed : %d", status));
		RTPCLOSE(_listenSock);
		_listenSock = RTPSOCKERR;
		delete sess;
		_hasStart = false;
		return -1;
	}
	return 0;
}

void IpcWithTcp::stop()
{
	if (_hasStart) {
		_hasStart = false;
		if (_listenSock != RTPSOCKERR) {
			RTPCLOSE(_listenSock);
			_listenSock = RTPSOCKERR;
		}
		for (std::vector<SocketType>::iterator it = _vecSocks.begin(); it != _vecSocks.end(); ++it) {
			RTPCLOSE(*it);
		}
		if (_thread) {
			pj_thread_join(_thread);
			pj_thread_destroy(_thread);
			_thread = NULL;
		}
		if (_threadRecv) {
			pj_thread_join(_threadRecv);
			pj_thread_destroy(_threadRecv);
			_threadRecv = NULL;
		}
		if (_rtpSession) {
			delete _rtpSession;
			_rtpSession = NULL;
		}
	}
}

int IpcWithTcp::acceptThread()
{
	while (_hasStart) {
		SocketType sock = accept(_listenSock, 0, 0);
		if (sock == RTPSOCKERR) {
			SIP_LOG(SIP_ERROR, (THIS_FILE, "accept socket failed, port:%d", _localRtpPort));
			return -1;
		}
		_vecSocks.push_back(sock);
		if (_rtpSession) {
			_rtpSession->AddDestination(RTPTCPAddress(sock));
		}
		RTPTime delay(0.010);
		RTPTime::Wait(delay);
	}
	return 0;
}
/*
 * create a thread to receive RTP packet
*/
void IpcWithTcp::onMediaUpdate(pjsip_inv_session *inv, pj_status_t status)
{
	status = pj_thread_create(_pool, "ReceiveThread", receive_rtp_thread, this, 0, 0, &_threadRecv);
	if (status != PJ_SUCCESS) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "create receive RTP thread failed : %d", status));
		return;
	}
}

/*
 *
*/
int IpcWithTcp::receiveThread()
{
	double sec = 0.015;
	RTPTime delay(sec);
	while (_hasStart) {
		_rtpSession->BeginDataAccess();
		if (_rtpSession->GotoFirstSourceWithData()) {
			do {
				jrtplib::RTPPacket *packet = NULL;
				while (_hasStart && (packet = _rtpSession->GetNextPacket()) != 0) {
					/*update timestamp indecate RTP is activated yet*/
					updateActivateTm();
					unsigned char* payload = packet->GetPayloadData();
					int paylen = packet->GetPayloadLength();
					// translate RTP payload into StreamMgr class
					StreamMgr::ins()->translateRtpPayload(this->_ipcAddr.c_str(), this->_ipcPort, reinterpret_cast<const char*>(payload), paylen);
					_rtpSession->DeletePacket(packet);
				}
			} while (_hasStart && _rtpSession->GotoNextSourceWithData());
		}
		_rtpSession->EndDataAccess();

#ifndef RTP_SUPPORT_THREAD
		int status = _rtpSession->Poll();
#endif // RTP_SUPPORT_THREAD
		RTPTime::Wait(delay);
	}
	return 0;
}

void IpcWithTcp::onSendError(SocketType sock)
{

}
void IpcWithTcp::onReceiveError(SocketType sock)
{
	_tcpTransmitter.DeleteDestination(RTPTCPAddress(sock));
}