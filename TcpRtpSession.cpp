#include "pch.h"
#include "TcpRtpSession.h"

TcpRtpSession::TcpRtpSession(unsigned char rtpChannel)
	: RTPSession()
{
	_rtpChannel = rtpChannel;
}

TcpRtpSession::~TcpRtpSession()
{
	
}
/*
void TcpRtpSession::SetChangeOutgoingData(bool change)
{
	RTPSession::SetChangeOutgoingData(change);
}
*/
/*
 * realloca a buffer to send that add RTP_TCP_header 
*/
/*
int TcpRtpSession::OnChangeRTPOrRTCPData(const void *origdata, size_t origlen, bool isrtp, void **senddata, size_t *sendlen)
{
	if (origdata && origlen > 0) {
		*sendlen = origlen + 4;
		*senddata = new char[*sendlen];
		char* p = (char*)*senddata;
		*p++ = 0x24;
		if (isrtp) {
			*p++ = _rtpChannel;
		}
		else {
			*p++ = (_rtpChannel + 1);
		}
		unsigned short len = htons(origlen);
		memcpy(p, (char*)&len, 2);
		p += 2;
		memcpy(p, origdata, origlen);
	}
	else {
		return -1;
	}
	return 0;
}

void TcpRtpSession::OnSentRTPOrRTCPData(void *senddata, size_t sendlen, bool isrtp)
{
	if (senddata) {
		delete[] (char*)senddata;
	}
}
*/