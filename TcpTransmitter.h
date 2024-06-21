#pragma once
#include "rtptcptransmitter.h"

using namespace jrtplib;

typedef void(*sendErrorCallback) (SocketType sock, void*);
typedef void(*receiveErrorCallback) (SocketType sock, void*);

class TcpTransmitter : public RTPTCPTransmitter
{
public:
	TcpTransmitter(const char* name);
	void setCallbackFunc(sendErrorCallback func1, receiveErrorCallback func2, void* ctx);
	virtual void OnSendError(SocketType sock);
	virtual void OnReceiveError(SocketType sock);
private:
	std::string _name;
	sendErrorCallback _sendErrorFunc;
	receiveErrorCallback _receiveErrorFunc;
	void *_context;
};