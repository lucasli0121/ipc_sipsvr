#pragma once
#include "rtpsession.h"
#include "TcpTransmitter.h"

/*
 * Provide real-time video forwarding service to send IPC pushed video flow to the server
*/
class LiveRtpCli
{
public:
	LiveRtpCli();
	virtual ~LiveRtpCli();
	int open(int port, const char* ipcAddr, const char* svrAddr, int svrPort);
	void close();
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	int getPort() { return _port; }
	int sendRtp(const char* ipcAddr, const char* payload, int paylen);
	void onSendError(SocketType sock);
	void onReceiveError(SocketType sock);
private:
	jrtplib::RTPSession * _rtpSession;
	std::string _ipcAddr;
	int _port;
	std::string _svrAddr;
	int _svrPort;
	bool _isOpen;
	SocketType _socket;
	TcpTransmitter _tcpTransmitter;
};
