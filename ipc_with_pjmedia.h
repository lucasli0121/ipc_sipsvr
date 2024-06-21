#pragma once
#include "ipc_call.h"

class IpcWithPjMedia : public IpcCall
{
public:
	IpcWithPjMedia(const char* deviceId, const char* ipcAddr, int ipcPort, int localRtpPort, uint32_t index);
	virtual int start(pj_caching_pool* cp);
	virtual void stop();
	virtual void onMediaUpdate(pjsip_inv_session *inv, pj_status_t status);
	void onAudioRxRtp(void *pkt, pj_ssize_t size);
	void onAudioRxRtcp(void *pkt, pj_ssize_t size);
	void onVideoRxRtp(void *pkt, pj_ssize_t size);
	void onVideoRxRtcp(void *pkt, pj_ssize_t size);
	pj_status_t createLocalSdp(pjsip_dialog* dlg, pjmedia_sdp_session** local_sdp);
private:
	pjmedia_endpt * _media_endpt;
	pjmedia_stream_info     _audioStreamInfo;		    /* Current stream info.	*/
	pjmedia_vid_stream_info _videoStreamInfo;
	pjmedia_rtp_session  _audioRtpSession;	    /* incoming RTP session	*/
	pjmedia_rtp_session	 _videoRtpSession;	    /* incoming RTP session	*/
												/* RTCP stats: */
	pjmedia_rtcp_session _audioRtcpSession;		    /* incoming RTCP session.	*/
	pjmedia_rtcp_session _videoRtcpSession;
};
