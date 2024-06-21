#pragma once

#include "pjproject.h"
#include "mqinf.h"
#include "config_mgr.h"
#include <string>
#include <vector>

#define MAX_MEDIA_CNT   1
#define AUDIO_MEDIA_IDX 0
#define VIDEO_MEDIA_IDX 0
/* A bidirectional media stream created when the call is active. */
struct media_stream_t
{
	/* Static: */
	unsigned				call_index;			/* Call owner.		*/
	unsigned				media_index;	    /* Media index in call.	*/
	pjmedia_transport		* transport;			/* To send/recv RTP/RTCP	*/
	pjmedia_transport_info	transport_info;	/**/

	pjmedia_sock_info		sock_info;	//media sock info
										/* Active? */
	pj_bool_t		 active;	    /* Non-zero if is in call.	*/
	unsigned		 clock_rate;	    /* clock rate		*/
	unsigned		 samples_per_frame; /* samples per frame	*/
	unsigned		 bytes_per_frame;   /* frame size.		*/
										/* RTP session: */

};

/* This is a call structure that is created when the application starts
* and only destroyed when the application quits.
*/
struct call_t
{
	unsigned			index;
	pjsip_inv_session	*inv;
	unsigned			media_count;
	media_stream_t		media[MAX_MEDIA_CNT];
	pj_time_val			start_time;
	pj_time_val			response_time;
	pj_time_val			connect_time;
};

/*
 *  media stream accept class for IPC
 *  onMediaUpdate() called by SipApp and in this function IpcCall call pjmedia_transport_attach() 
 * pjmedia_transport_attach() attach on_video_rx_rtp() to receive media RTP packet from IPC.
*/
class IpcCall
{
public:
	IpcCall(const char* deviceId, const char* ipcAddr, int ipcPort, int localRtpPort, uint32_t index);
	virtual int start(pj_caching_pool* cp) { return 0;  }
	virtual void stop() {}
	virtual void onMediaUpdate(pjsip_inv_session *inv, pj_status_t status) {}
	virtual ~IpcCall() {}
	bool isStart() { return _hasStart; }
	struct call_t* getCall() { return &this->_call; }
	std::string getDeviceId() { return _deviceId; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	std::string getRecvAddr();
	int getLocalRtpPort() { return _localRtpPort; }
	int getIpcPort() { return _ipcPort; }
	void setSdpAttr(int frame, int remoteRtpPort, const char* fmt = "ps");
	long getInActivateTmLen();
	virtual pj_pool_t* getPool() {
		return _pool;
	}
protected:
	void updateActivateTm();
protected:
	struct call_t _call;
	std::string _deviceId;
	int	_localRtpPort;
	std::string		_ipcAddr;
	pj_bool_t		_hasStart;
	int _frame;
	int _remoteRtpPort;
	int _ipcPort;
	std::string _fmt;
	pj_time_val _activeTm;
	pj_pool_t* _pool;
};
