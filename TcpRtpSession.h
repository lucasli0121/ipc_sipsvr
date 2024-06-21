#pragma once
#include "rtpsession.h"

using namespace jrtplib;

class TcpRtpSession : public RTPSession
{
public:
	TcpRtpSession(unsigned char rtpChannel = 0);
	~TcpRtpSession();
	//void SetChangeOutgoingData(bool change);
private:
	/*
	virtual int OnChangeRTPOrRTCPData(const void *origdata, size_t origlen, bool isrtp, void **senddata, size_t *sendlen);
	virtual void OnSentRTPOrRTCPData(void *senddata, size_t sendlen, bool isrtp);*/
private:
	unsigned char _rtpChannel;
};
