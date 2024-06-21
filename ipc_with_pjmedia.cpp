/*
* IpcCall.cpp ��
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h" // NOLINT
#include "ps_codec.h" // NOLINT
#include "stream_mgr.h" // NOLINT
#include "ipc_with_pjmedia.h" // NOLINT

static const char* THIS_FILE = "IpcWithPjMedia";
/*
* This callback is called by media transport on receipt of RTP packet.
*/
static void on_audio_rx_rtp(void *user_data, void *pkt, pj_ssize_t size)
{
	IpcWithPjMedia* pCall = reinterpret_cast<IpcWithPjMedia*>(user_data);
	if (pCall) {
		pCall->onAudioRxRtp(pkt, size);
	}
}

/*
* This callback is called by media transport on receipt of RTCP packet.
*/
static void on_audio_rx_rtcp(void *user_data, void *pkt, pj_ssize_t size)
{
	IpcWithPjMedia* pCall = reinterpret_cast<IpcWithPjMedia*>(user_data);
	if (pCall) {
		pCall->onAudioRxRtcp(pkt, size);
	}
}

/*
* This callback is called by media transport on receipt of RTP packet.
*/
static void on_video_rx_rtp(void *user_data, void *pkt, pj_ssize_t size)
{
	IpcWithPjMedia* pCall = reinterpret_cast<IpcWithPjMedia*>(user_data);
	if (pCall) {
		pCall->onVideoRxRtp(pkt, size);
	}
}

/*
* This callback is called by media transport on receipt of RTCP packet.
*/
static void on_video_rx_rtcp(void *user_data, void *pkt, pj_ssize_t size)
{
	IpcWithPjMedia* pCall = reinterpret_cast<IpcWithPjMedia*>(user_data);
	if (pCall) {
		pCall->onVideoRxRtcp(pkt, size);
	}
}

IpcWithPjMedia::IpcWithPjMedia(const char* deviceId, const char* ipcAddr, int ipcPort, int localRtpPort, uint32_t index)
	: IpcCall(deviceId, ipcAddr, ipcPort, localRtpPort, index)
{
	_media_endpt = NULL;
	memset(&_audioStreamInfo, 0, sizeof(_audioStreamInfo));
	memset(&_videoStreamInfo, 0, sizeof(_videoStreamInfo));
	memset(&_audioRtpSession, 0, sizeof(_audioRtpSession));
	memset(&_videoRtpSession, 0, sizeof(_videoRtpSession));
	memset(&_audioRtcpSession, 0, sizeof(_audioRtcpSession));
	memset(&_videoRtcpSession, 0, sizeof(_videoRtcpSession));
}

/*
* start rtp endpt ready to accept IPC video stream
* cp: sip 
* remotePort: IPC remote RTP port
* frame: IPC's real frame
* fmt: PS ( or H264...)
*/
int IpcWithPjMedia::start(pj_caching_pool* cp)
{
	TRACE_FUNC("IpcWithPjMedia::start()");
	pj_status_t status = PJ_SUCCESS;
	if (_hasStart) {
		return 0;
	}
	_pool = pj_pool_create(&(cp->factory), "pjmediaCall", 1024, 1024, NULL);
	status = pjmedia_endpt_create(&(cp->factory), NULL, 1, &_media_endpt);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "pjmedia_endpt_create file", status);
		return -1;
	}
	_call.media_count = MAX_MEDIA_CNT;
	for (int i = 0; i < MAX_MEDIA_CNT; i++) {
		status = pjmedia_transport_udp_create3(_media_endpt,
			pj_AF_INET(),
			"mediartp",
			NULL,
			_localRtpPort + i * 2,
			0,
			&(_call.media[i].transport));
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "pjmedia_transport_udp_create2 call failed", status);
			if (_media_endpt) {
				pjmedia_endpt_destroy(_media_endpt);
				_media_endpt = NULL;
			}
			return -1;
		}
		pjmedia_transport_info_init(&(_call.media[i].transport_info));
		pjmedia_transport_get_info(_call.media[i].transport, &(_call.media[i].transport_info));
		pj_memcpy(&_call.media[i].sock_info, &_call.media[i].transport_info.sock_info, sizeof(pjmedia_sock_info));
	}
	_hasStart = true;
	return 0;
}
/*
 * stop pjmedia rtp
*/
void IpcWithPjMedia::stop()
{
	TRACE_FUNC("IpcWithPjMedia::stop()");
	if (_hasStart) {
		_hasStart = false;
		StreamMgr::ins()->removeIpcChannel(getIpcAddr(), getIpcPort());
		for (int i = 0; i < MAX_MEDIA_CNT; i++) {
			struct media_stream_t *stream = &_call.media[i];
			if (stream && stream->transport) {
				stream->active = PJ_FALSE;
				//pjmedia_transport_detach(stream->transport, stream);
				pjmedia_transport_close(stream->transport);
				stream->transport = NULL;
			}
		}
	}
	if (_media_endpt) {
		pjmedia_endpt_destroy(_media_endpt);
		_media_endpt = NULL;
	}
	if (_pool) {
		pj_pool_release(_pool);
		_pool = NULL;
	}
}

/*
* will called by sipapp
* this function initialize media ports and start receive RTP packets
*/
void IpcWithPjMedia::onMediaUpdate(pjsip_inv_session *inv, pj_status_t status)
{
	TRACE_FUNC("IpcWithPjMedia::onMediaUpdate");

	if (!_hasStart) {
		SIP_LOG(SIP_INFO, (THIS_FILE, "Ipc media is not start yet"));
		return;
	}
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "SDP negotiation failed", status);
		return;
	}
	// struct media_stream_t *audio_media;
	struct media_stream_t *video_media;
	const pjmedia_sdp_session *local_sdp, *remote_sdp;
	pjmedia_sdp_neg_get_active_local(inv->neg, &local_sdp);
	pjmedia_sdp_neg_get_active_remote(inv->neg, &remote_sdp);
	video_media = &_call.media[VIDEO_MEDIA_IDX];
	if( ! video_media->active)  {
		status = pjmedia_vid_stream_info_from_sdp(&_videoStreamInfo, _pool ? _pool : inv->pool, _media_endpt, local_sdp, remote_sdp, VIDEO_MEDIA_IDX);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "Error creating stream info from SDP", status);
			return;
		}
		int clockRate = ConfigMgr::ins()->getClockRate();
		int bitRate = ConfigMgr::ins()->getBitRate();
		video_media->clock_rate = clockRate;
		video_media->samples_per_frame = video_media->clock_rate / _frame;//video_media->clock_rate * frame / 1000;
		video_media->bytes_per_frame = bitRate * _frame / 1000;
		pjmedia_rtp_session_init(&_videoRtpSession, _videoStreamInfo.rx_pt, _videoStreamInfo.ssrc);
		pjmedia_rtcp_init(&_videoRtcpSession, const_cast<char*>("videoRtcp"), video_media->clock_rate, video_media->samples_per_frame, _videoStreamInfo.ssrc);
		/* Attach media to transport */
		status = pjmedia_transport_attach(video_media->transport,
			this,
			&_videoStreamInfo.rem_addr,
			&_videoStreamInfo.rem_rtcp,
			sizeof(pj_sockaddr_in),
			&on_video_rx_rtp,
			&on_video_rx_rtcp);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "Error on pjmedia_transport_attach()", status);
			return;
		}
		/* Start media transport */
		status = pjmedia_transport_media_start(video_media->transport, 0, 0, 0, VIDEO_MEDIA_IDX);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "Error on pjmedia_transport_media_start()", status);
			return;
		}
		video_media->active = PJ_TRUE;
	}
	
	// get format of media stream.
	std::string ext("ps");
	if (_videoStreamInfo.codec_info.fmt_id == PJMEDIA_FORMAT_PS) {
		ext = "ps";
	}
	// set IpcChannel again if _ipcAddr is null yet
	if (_ipcAddr.length() == 0) {
		char val[255];
		memset(val, 0, sizeof(val));
		pj_sockaddr_print(&_videoStreamInfo.rem_addr, val, 255, 0);
		_ipcAddr = val;
		StreamMgr::ins()->addIpcChannel(_ipcAddr.c_str(), _ipcPort, ext.c_str());
	} else {
		StreamMgr::ins()->setFmt(_ipcAddr.c_str(), _ipcPort, ext.c_str());
	}
	pj_gettimeofday(&_activeTm);
}
/*
* handle andio rtp rx event
*/
void IpcWithPjMedia::onAudioRxRtp(void *pkt, pj_ssize_t size)
{
	struct media_stream_t *stream;
	pj_status_t status;
	const pjmedia_rtp_hdr *hdr;
	const void *payload;
	unsigned payload_len;

	stream = &(this->_call.media[AUDIO_MEDIA_IDX]);

	/* Discard packet if media is inactive */
	if (!stream->active)
		return;

	/* Check for errors */
	if (size < 0) {
		ERROR_LOG(THIS_FILE, "RTP recv() error", (pj_status_t)-size);
		return;
	}
	/* Decode RTP packet. */
	status = pjmedia_rtp_decode_rtp(&_audioRtpSession, pkt, static_cast<int>(size), &hdr, &payload, &payload_len);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "RTP decode error", status);
		return;
	}
	// SIP_LOG(DEBUG,(THIS_FILE, "Rx seq=%d", pj_ntohs(hdr->seq)));
	/* Update the RTCP session. */
	pjmedia_rtcp_rx_rtp(&_audioRtcpSession, pj_ntohs(hdr->seq), pj_ntohl(hdr->ts), payload_len);
	/* Update RTP session */
	pjmedia_rtp_session_update(&_audioRtpSession, hdr, NULL);
}
/*
* rtcp event message
*/
void IpcWithPjMedia::onAudioRxRtcp(void *pkt, pj_ssize_t size)
{
	struct media_stream_t *stream = &(this->_call.media[AUDIO_MEDIA_IDX]);
	/* Discard packet if media is inactive */
	if (!stream->active)
		return;
	/* Check for errors */
	if (size < 0) {
		ERROR_LOG(THIS_FILE, "Error receiving RTCP packet", (pj_status_t)-size);
		return;
	}
	/* Update RTCP session */
	pjmedia_rtcp_rx_rtcp(&_audioRtcpSession, pkt, size);
}
/*
* video rtp rx
* This function is called back when received RTP packet
* we can save or translate RTP packet in the function
*/
void IpcWithPjMedia::onVideoRxRtp(void *pkt, pj_ssize_t size)
{
	struct media_stream_t *stream;
	pj_status_t status;
	const pjmedia_rtp_hdr *hdr;
	const void *payload;
	unsigned payload_len;
	/*update timestamp indecate RTP is activated yet*/
	updateActivateTm();
	stream = &(this->_call.media[VIDEO_MEDIA_IDX]);
	/* Discard packet if media is inactive */
	//if (!stream->active)
	//	return;
	/* Check for errors */
	if (size < 0) {
		ERROR_LOG(THIS_FILE, "RTP recv() error", (pj_status_t)-size);
		return;
	}
	/* Decode RTP packet. */
	status = pjmedia_rtp_decode_rtp(&_videoRtpSession, pkt, static_cast<int>(size), &hdr, &payload, &payload_len);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "RTP decode error", status);
		return;
	}
	
	// translate RTP payload into StreamMgr class
	StreamMgr::ins()->translateRtpPayload(this->_ipcAddr.c_str(), this->_ipcPort, static_cast<const char*>(payload), payload_len);
	/* Update the RTCP session. */
	pjmedia_rtcp_rx_rtp(&_videoRtcpSession, pj_ntohs(hdr->seq), pj_ntohl(hdr->ts), payload_len);
	/* Update RTP session */
	pjmedia_rtp_session_update(&_videoRtpSession, hdr, NULL);
}
void IpcWithPjMedia::onVideoRxRtcp(void *pkt, pj_ssize_t size)
{
	struct media_stream_t *stream = &(this->_call.media[VIDEO_MEDIA_IDX]);
	/* Discard packet if media is inactive */
	if (!stream->active)
		return;
	/* Check for errors */
	if (size < 0) {
		ERROR_LOG(THIS_FILE, "Error receiving RTCP packet", (pj_status_t)-size);
		return;
	}
	/* Update RTCP session */
	pjmedia_rtcp_rx_rtcp(&_videoRtcpSession, pkt, size);
}

pj_status_t IpcWithPjMedia::createLocalSdp(pjsip_dialog* dlg, pjmedia_sdp_session** local_sdp)
{
	pjmedia_sock_info sockInfo[MAX_MEDIA_CNT];
	for (int i = 0; i < MAX_MEDIA_CNT; i++) {
		pj_memcpy(&sockInfo[i], &_call.media[i].sock_info, sizeof(pjmedia_sock_info));
	}

	pj_status_t status = pjmedia_endpt_create_sdp(_media_endpt,	    /* the media endpt	*/
		dlg->pool,	    /* pool.		*/
		MAX_MEDIA_CNT,   /* # of streams	*/
		sockInfo,     /* RTP sock info	*/
		local_sdp);	    /* the SDP result	*/
	return status;
}
