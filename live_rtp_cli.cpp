/*
* live_rtp_cli.cpp ��
* Copyright [2019] <liguoqiang>"
* 
*/

#include "pch.h"   // NOLINT
#include "rtpsession.h" // NOLINT
#include "rtpudpv4transmitter.h" // NOLINT
#include "rtpipv4address.h" // NOLINT
#include "rtpsessionparams.h" // NOLINT
#include "rtperrors.h" // NOLINT
#include "rtplibraryversion.h" // NOLINT
#include "rtppacket.h" // NOLINT
#include "rtptcpaddress.h" // NOLINT
#include "rtpsocketutil.h" // NOLINT
#include "rtpsocketutilinternal.h" // NOLINT
#include "TcpRtpSession.h"
#include "live_rtp_cli.h" // NOLINT

using namespace jrtplib;
static const char* THIS_FILE = "live_rtp_cli.cpp";
#define RTP_PACK_SIZE (1400)

static void onSendErrorFunc(SocketType sock, void* arg)
{
	LiveRtpCli * obj = reinterpret_cast<LiveRtpCli*>(arg);
	if (obj) {
		obj->onSendError(sock);
	}
}
static void onReceiveErrorFunc(SocketType sock, void* arg)
{
	LiveRtpCli * obj = reinterpret_cast<LiveRtpCli*>(arg);
	if (obj) {
		obj->onReceiveError(sock);
	}
}

LiveRtpCli::LiveRtpCli()
	: _tcpTransmitter("LiveRtpCli")
{
	_isOpen = false;
	_socket = RTPSOCKERR;
}

LiveRtpCli::~LiveRtpCli()
{
	close();
}

/*
 * open the RTP forward service
 * port: local RTP port
 * ipcAddr: the address of IPC
 * svrAddr: the address of RTP server
 * svrPort: RTP port of server
*/
int LiveRtpCli::open(int port, const char* ipcAddr, const char* svrAddr, int svrPort)
{
	TRACE_FUNC(" LiveRtpCli::open()");

	if (!_isOpen) {
		_port = port;
		_ipcAddr = ipcAddr;
		_svrAddr = svrAddr;
		_svrPort = svrPort;
		
		int status = 0;
		_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (_socket == RTPSOCKERR) {
			SIP_LOG(SIP_ERROR, (THIS_FILE, " create client socket error"));
			return -1;
		}
		struct sockaddr_in servAddr;
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(svrPort);
		servAddr.sin_addr.s_addr = inet_addr(svrAddr);
		if (connect(_socket, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0) {
			SIP_LOG(SIP_ERROR, (THIS_FILE, " connect to RTP server error, ip=%s, port=%d", svrAddr, svrPort));
			RTPCLOSE(_socket);
			_socket = RTPSOCKERR;
			return -1;
		}

		bool threadsafe = false;
#ifdef RTP_SUPPORT_THREAD
		threadsafe = true;
#endif // RTP_SUPPORT_THREAD
		_tcpTransmitter.Init(threadsafe);
		_tcpTransmitter.Create(65535, 0);
		_tcpTransmitter.setCallbackFunc(onSendErrorFunc, onReceiveErrorFunc, this);
		RTPSessionParams sessparams;
		sessparams.SetOwnTimestampUnit(1.0 / 90000.0);
		sessparams.SetProbationType(RTPSources::NoProbation);
		sessparams.SetAcceptOwnPackets(true);
		sessparams.SetUsePredefinedSSRC(true);
		sessparams.SetPredefinedSSRC(200);
		TcpRtpSession * sess = new TcpRtpSession();
		_rtpSession = sess;
		status = _rtpSession->Create(sessparams, &_tcpTransmitter);
		if (status < 0) {
			SIP_LOG(SIP_ERROR, (THIS_FILE, "rtp session create error:%s\n", RTPGetErrorString(status).c_str()));
			return -1;
		}
		_rtpSession->AddDestination(RTPTCPAddress(_socket));
		_rtpSession->SetDefaultPayloadType(96); // H264
		_rtpSession->SetDefaultMark(true);
		_rtpSession->SetDefaultTimestampIncrement(10);
		_rtpSession->SetMaximumPacketSize(RTP_PACK_SIZE + 12); // RTP max size 
		_isOpen = true;
	}
	return 0;
}

void LiveRtpCli::close()
{
	TRACE_FUNC(" LiveRtpCli::close()");
	if (_isOpen) {
		_isOpen = false;
		if (_socket != RTPSOCKERR) {
			RTPCLOSE(_socket);
			_socket = RTPSOCKERR;
		}
		if (_rtpSession) {
			_rtpSession->Destroy();
			delete _rtpSession;
			_rtpSession = NULL;
		}
		_tcpTransmitter.Destroy();
	}
}
/*
 *  Send RTP packet to destination
*  and datermine whether the IPC address matches before sending.
*/
int LiveRtpCli::sendRtp(const char* ipcAddr, const char* payload, int paylen)
{
	//TRACE_FUNC(" LiveRtpCli::sendRtp()");
	int ret = -1;
	if (!_isOpen) {
		return ret;
	}
	if (ipcAddr && strcmp(ipcAddr, _ipcAddr.c_str()) == 0) {
		if (payload && paylen > 0) {
			while (paylen > 0) {
				int len = (paylen > RTP_PACK_SIZE ? RTP_PACK_SIZE : paylen);
				ret = _rtpSession->SendPacket(payload, len);
				if (ret < 0) {
					SIP_LOG(SIP_ERROR, ("send rtp packet error,%s\n", RTPGetErrorString(ret).c_str()));
				}
				payload += len;
				paylen -= len;
			}
			/*
			_rtpSession->BeginDataAccess();
			if (_rtpSession->GotoFirstSourceWithData()) {
				do {
					jrtplib::RTPPacket *packet = NULL;
					while ((packet = _rtpSession->GetNextPacket()) != 0 ) {
						unsigned char* data = packet->GetPayloadData();
						int datalen = packet->GetPayloadLength();
						SIP_LOG(SIP_DEBUG, (THIS_FILE, "receive packet length:%d\n", paylen));
						_rtpSession->DeletePacket(packet);
					}
				} while (_rtpSession->GotoNextSourceWithData());
			}
			_rtpSession->EndDataAccess();
			*/
#ifndef RTP_SUPPORT_THREAD
			//int status = _rtpSession->Poll();
#endif // RTP_SUPPORT_THREAD
		}
	}
	return ret;
}

void LiveRtpCli::onSendError(SocketType sock)
{
	TRACE_FUNC("LiveRtpCli::onSendError()");
	if (sock == RTPSOCKERR) {
		return;
	}
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(_svrPort);
	servAddr.sin_addr.s_addr = inet_addr(_svrAddr.c_str());
	if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, " connect to RTP server error, ip=%s, port=%d", _svrAddr.c_str(), _svrPort));
		_tcpTransmitter.DeleteDestination(RTPTCPAddress(sock));
		LiveStopReq obj;
		obj.setSvcAddr(_svrAddr.c_str());
		obj.setIpcAddr(_ipcAddr.c_str());
		obj.setStartPort(_port);
		obj.setEndPort(_port + 1);
		MqInf::ins()->publishMsg(LIVE_STOP_REQ, &obj);
		return;
	}
}
void LiveRtpCli::onReceiveError(SocketType sock)
{
	
}
