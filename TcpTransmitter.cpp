/*
* TcpTransmitter.cpp ¡£
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h"
#include "rtptcpaddress.h" // NOLINT
#include "TcpTransmitter.h"

static const char* THIS_FILE = "TcpTransmitter";


TcpTransmitter::TcpTransmitter(const char* name)
	: RTPTCPTransmitter(NULL)
{
	if (name) _name = name;
	_sendErrorFunc = NULL;
	_receiveErrorFunc = NULL;
	_context = NULL;
}

void TcpTransmitter::setCallbackFunc(sendErrorCallback func1, receiveErrorCallback func2, void* ctx)
{
	_sendErrorFunc = func1;
	_receiveErrorFunc = func2;
	_context = ctx;
}

void TcpTransmitter::OnSendError(SocketType sock)
{
	if (_sendErrorFunc) {
		_sendErrorFunc(sock, _context);
	}
	//DeleteDestination(RTPTCPAddress(sock));
}

void TcpTransmitter::OnReceiveError(SocketType sock)
{
	if (_receiveErrorFunc) {
		_receiveErrorFunc(sock, _context);
	}
	//DeleteDestination(RTPTCPAddress(sock));
}