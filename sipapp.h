#pragma once
#include "pjproject.h"
#include "objs.h"
#include "mqinf.h"
#include "ipc_call.h"
#include "live_rtp_cli.h"
#include <map>
#include <list>
#include <vector>

class SipApp : public MqCallback
{
	enum HeadType { NoHead = 0, AuthHead, DateHead};
	struct response_data_t {
		pjsip_tx_data *_data;
		pjsip_response_addr _addr;
		char* buf;
	};

	typedef std::map<uint32_t, response_data_t*> RspDataMap;
	typedef std::list<IpcCall*> CallList;
private:
	int	_sip_port;
	pj_str_t _localAddr;
	pj_str_t _localUri;
	pj_str_t _localContact;
	pj_caching_pool *_cp;
	pjsip_endpoint	*_sip_endpt;
	pjsip_transport *_sip_tp;
	pj_bool_t		 _thread_quit;
	pj_thread_t		*_work_thread;
	pj_thread_t     *_alive_thread;

	
	bool _has_start;
	CallList		_lstCall;
	pj_lock_t		*_lockCall;
	RspDataMap		_mapRspData;
	pj_lock_t *		_sipLock;
	pj_lock_t*      _msgLock;
	uint32_t		_snVal;
	pj_lock_t      *_stopLock;
	pj_lock_t      *_reqLock;
	int				_rtpNextPort;
	std::string		_sipId;
	std::vector<codec> _vecCodec;
public:
	SipApp();
	~SipApp();
	void init(pj_caching_pool *);
	void unini();
	pj_status_t start(int argc, char** argv);
	void stop();
	int handle_work_thread_event();
	int handleAliveThread();
	pj_bool_t onRxRequest(pjsip_rx_data *data);
	pj_bool_t onRxResponse(pjsip_rx_data *data);
	pj_bool_t onTxResponse(pjsip_tx_data *data);
	void onMsgCallback(const char* topic, MqObject* obj);
	void handleAuthEvent(pjsip_rx_data *rdata, pjsip_authorization_hdr* authHdr);
	pj_status_t createWorkThread(pj_pool_t*, const char* name, pj_thread_proc *proc, void *arg, pj_thread_t **thread);
	void onStateChanged(pjsip_inv_session *inv, pjsip_event *e);
	void onNewSession(pjsip_inv_session *inv, pjsip_event *e);
	void onMediaUpdate(pjsip_inv_session *inv, pj_status_t status);
	void onRxOffer(pjsip_inv_session *inv, const pjmedia_sdp_session *offer);
	void onSendRequestCallback(pjsip_send_state *st, pj_ssize_t sent, pj_bool_t *cont);
private:
	void reportIpcStatus(const char* ipcAddr, int port, int status);
	void handleRegisterRequest(pj_pool_t* pool, pjsip_rx_data* rdata);
	void handleOtherMessageRequest(pj_pool_t* pool, pjsip_rx_data* rdata);
	void hangupAll(void);
	void responseToIpc(pjsip_tx_data* tdata, pjsip_response_addr *addr, HeadType headType);
	uint32_t createSn();
	std::string createSdp(IpcCall* pCall);
	pj_bool_t sendSdpToIpc2(pjsip_rx_data* rdata, IpcCall* obj);
	unsigned int createNextRtpPort();
	void destroyCallMedia(unsigned int index);
	pjmedia_sdp_session *createAnswer(pj_pool_t* pool, const pjmedia_sdp_session *offer);
	pjmedia_sdp_attr * removeSdpAttrs(unsigned *cnt, pjmedia_sdp_attr *attr[], unsigned cnt_attr_to_remove, const char* attr_to_remove[]);
	LiveRtpCli* makeLiveRtpClient(LiveMediaStart* obj);
	void removeLiveRtpClient(LiveStopReq* obj);
	IpcCall* findCallByUri(const char* ipcUri);
	IpcCall* findCallByAddrAndPort(const char* ipcAddr, int port);
	IpcCall* findCallByIndex(unsigned int index);
	void sendPTZ2Ipc(PTZCmd* obj);
	void sendPTZ2IpcStop(const char* ipcIp, int port);
	void sendForceKeyFrame(struct call_t* call);
};
