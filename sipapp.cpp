/*
 * sipapp.cpp ��
 * Copyright [2019] <liguoqiang>"
 */

#include "pch.h" // NOLINT
#include "sipapp.h" // NOLINT
#include "ps_codec.h" // NOLINT
#include "live_rtp_cli.h" // NOLINT
#include "ipc_with_pjmedia.h" // NOLINT
#include "ipc_with_tcp.h" // NOLINT

#ifndef CHECK_STATUS
#define CHECK_STATUS do { if (status != PJ_SUCCESS)  return status; } while (0)
#endif

static char _addBuf[32];
static char _urlBuf[64];
static char _contactBuf[64];
static char _poolBuf[1024];
static const char* THIS_FILE = "sipapp.cpp";
static SipApp * _sipApp = NULL;

const pjsip_method pjsip_message_method =
{
	(pjsip_method_e)PJSIP_OTHER_METHOD,
	{ const_cast<char*>("MESSAGE"), 7 }
};

static const pjsip_method pjsip_ask_method =
{
	(pjsip_method_e)PJSIP_ACK_METHOD,
	{ const_cast<char*>("ASK"), 3 }
};

static pj_status_t pjsip_auth_lookup(pj_pool_t *pool, const pj_str_t *realm, const pj_str_t *acc_name, pjsip_cred_info *cred_info);

static pj_status_t pjsip_auth_lookup(pj_pool_t *pool, const pj_str_t *realm, const pj_str_t *acc_name, pjsip_cred_info *cred_info)
{
	pj_status_t status = PJ_SUCCESS;
	return status;
}

static void on_state_changed(pjsip_inv_session *inv, pjsip_event *e)
{
	_sipApp->onStateChanged(inv, e);
}
/* Callback to be called when dialog has forked: */
static void on_new_session(pjsip_inv_session *inv, pjsip_event *e)
{
	_sipApp->onNewSession(inv, e);
}
/* Callback to be called when SDP negotiation is done in the call: */
static void on_media_update(pjsip_inv_session *inv, pj_status_t status)
{
	_sipApp->onMediaUpdate(inv, status);
}

static void on_rx_offer(pjsip_inv_session *inv, const pjmedia_sdp_session *offer)
{
	_sipApp->onRxOffer(inv, offer);
}

static pj_bool_t on_rx_request(pjsip_rx_data *rdata)
{
	return _sipApp->onRxRequest(rdata);
}

static pj_bool_t on_rx_response(pjsip_rx_data *rdata)
{
	return _sipApp->onRxResponse(rdata);
}
static pj_bool_t on_tx_response(pjsip_tx_data* data)
{
	return _sipApp->onTxResponse(data);
}
/*
 * work thread handle sip event
*/
static int worker_thread_proc(void *arg)
{
	SipApp * app = reinterpret_cast<SipApp*>(arg);

	return app->handle_work_thread_event();
}
static int check_alive_thread(void * arg)
{
	SipApp * app = reinterpret_cast<SipApp*>(arg);
	return app->handleAliveThread();
}
static pjsip_module app_module =
{
	NULL, NULL,			    /* prev, next.		*/
	{ const_cast<char*>("sipsvr-app"), 10 },		    /* Name.			*/
	-1,				    /* Id			*/
	PJSIP_MOD_PRIORITY_APPLICATION, /* Priority			*/
	NULL,			    /* load()			*/
	NULL,			    /* start()			*/
	NULL,			    /* stop()			*/
	NULL,			    /* unload()			*/
	&on_rx_request,			    /* on_rx_request()		*/
	&on_rx_response,			 /* on_rx_response()		*/
	NULL,			    /* on_tx_request.		*/
	&on_tx_response,			    /* on_tx_response()		*/
	NULL				/* on_tsx_state()		*/
};

SipApp::SipApp()
{
	_sip_port = 5060;
	memset(_addBuf, 0, sizeof(_addBuf));
	memset(_urlBuf, 0, sizeof(_urlBuf));
	memset(_contactBuf, 0, sizeof(_contactBuf));
	memset(_poolBuf, 0, sizeof(_poolBuf));
	_localAddr.ptr = _addBuf;
	_localAddr.slen = 0;
	_localUri.ptr = _urlBuf;
	_localUri.slen = 0;
	_localContact.ptr = _contactBuf;
	_localContact.slen = 0;
	_cp = NULL;
	_sip_endpt = NULL;
	_has_start = false;
	_sipLock = NULL;
	_lockCall = NULL;
	_msgLock = NULL;
	_stopLock = NULL;
	_reqLock = NULL;
	_snVal = 0;
	_rtpNextPort = 4000; // init rpt port with RTP_START_PORT
	_sipId = "34020000002000000001";
	_sipApp = this;
	_sip_tp = NULL;
}

SipApp::~SipApp()
{
}
/*
 * init sip stack ,log ..
 * call only once
*/
void SipApp::init(pj_caching_pool *cp)
{
	_cp = cp;
	if (ConfigMgr::ins()->getVideoCodec(_vecCodec) == -1) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "cann't get video codec from config-manager"));
	}
	_sipId = ConfigMgr::ins()->getSipId();
	_sip_port = ConfigMgr::ins()->getSipPort();
	_rtpNextPort = ConfigMgr::ins()->getStartRtpPort();
	const char* sipHost = ConfigMgr::ins()->getSipHost();
	if (strcmp(sipHost, "0.0.0.0") != 0) {
		_localAddr.ptr = (char*)ConfigMgr::ins()->getSipHost();
		_localAddr.slen = strlen(ConfigMgr::ins()->getSipHost());
	}
}

void SipApp::unini()
{
	TRACE_FUNC("SipApp::unini()");
}
/*
 * start
 * bind udp port and start sip server 
 * create thread receives RX-request event.
*/
pj_status_t SipApp::start(int argc, char** argv)
{
	pj_status_t status = PJ_SUCCESS;
	if (!_has_start) {
		status = pjsip_endpt_create(&_cp->factory, pj_gethostname()->ptr, &_sip_endpt);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "pjsip_endpt_create failed", status);
			return status;
		}
		pj_pool_t* pool = pj_pool_create_on_buf("SipApp", _poolBuf, sizeof(_poolBuf));
		/* Add UDP transport. */
		{
			pj_sockaddr_in addr;
			pjsip_host_port addrname;
			pjsip_tpfactory *tpf = NULL;
			pj_bzero(&addr, sizeof(addr));
			addr.sin_family = pj_AF_INET();
			addr.sin_port = pj_htons((pj_uint16_t)_sip_port);
			if (_localAddr.slen) {
				addrname.host = _localAddr;
				addrname.port = _sip_port;
				status = pj_sockaddr_in_init(&addr, &_localAddr, (pj_uint16_t)_sip_port);
				if (status != PJ_SUCCESS) {
					ERROR_LOG(THIS_FILE, "Unable to resolve IP interface", status);
					return status;
				}
			}
			/*if use TCP call tcp-transport fuction*/
			if (ConfigMgr::ins()->getSipUseTcp()) {
				status = pjsip_tcp_transport_start2(_sip_endpt, &addr, (_localAddr.slen ? &addrname : NULL), 1, &tpf);
				if (status != PJ_SUCCESS) {
					ERROR_LOG(THIS_FILE, "Unable to start UDP transport", status);
					return status;
				}
				memcpy(_localAddr.ptr, tpf->addr_name.host.ptr, tpf->addr_name.host.slen);
				_localAddr.slen = tpf->addr_name.host.slen;
			} else {
				status = pjsip_udp_transport_start(_sip_endpt, &addr, (_localAddr.slen ? &addrname : NULL), 1, &_sip_tp);
				if (status != PJ_SUCCESS) {
					ERROR_LOG(THIS_FILE, "Unable to start UDP transport", status);
					return status;
				}
				memcpy(_localAddr.ptr, _sip_tp->local_name.host.ptr, _sip_tp->local_name.host.slen);
				_localAddr.slen = _sip_tp->local_name.host.slen;
			}
			char localAddr[32];
			char val[64];
			memset(localAddr, 0, sizeof(localAddr));
			memcpy(localAddr, _localAddr.ptr, _localAddr.slen);
			pj_ansi_sprintf(val, "<sip:%s@%s:%d>", _sipId.c_str(), localAddr, _sip_port);
			pj_ansi_strcpy(_localUri.ptr, val);
			_localUri.slen = strlen(val);
			pj_ansi_strcpy(_localContact.ptr, val);
			_localContact.slen = strlen(val);
			if (ConfigMgr::ins()->getSipUseTcp()) {
				SIP_LOG(SIP_INFO, (THIS_FILE, "SIP TCP listening on %.*s:%d", static_cast<int>(tpf->addr_name.host.slen), tpf->addr_name.host.ptr, tpf->addr_name.port));
			} else {
				SIP_LOG(SIP_INFO, (THIS_FILE, "SIP UDP listening on %.*s:%d", static_cast<int>(_sip_tp->local_name.host.slen), _sip_tp->local_name.host.ptr, _sip_tp->local_name.port));
			}
		}
		pjsip_tsx_layer_init_module(_sip_endpt);
		pjsip_ua_init_module(_sip_endpt, NULL);
		
		/* Init the callback for INVITE session: */
		pjsip_inv_callback inv_cb;
		pj_bzero(&inv_cb, sizeof(inv_cb));
		inv_cb.on_state_changed = &on_state_changed;
		inv_cb.on_new_session = &on_new_session;
		inv_cb.on_media_update = &on_media_update;
		inv_cb.on_rx_offer = &on_rx_offer;
		/* Initialize invite session module:  */
		status = pjsip_inv_usage_init(_sip_endpt, &inv_cb);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "pjsip_inv_usage_init failed", status);
			return status;
		}
		status = pjsip_100rel_init_module(_sip_endpt);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "pjsip_inv_usage_init failed", status);
			return status;
		}
		/* register invite request module */
		status = pjsip_endpt_register_module(_sip_endpt, &app_module);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "register module failed", status);
			return status;
		}
		pj_lock_create_simple_mutex(pool, NULL, &_sipLock);
		pj_lock_create_simple_mutex(pool, NULL, &_lockCall);
		pj_lock_create_simple_mutex(pool, NULL, &_msgLock);     // Lock for onMsgCallback
		pj_lock_create_simple_mutex(pool, NULL, &_stopLock); // Lock for stop sip
		pj_lock_create_simple_mutex(pool, NULL, &_reqLock);  // request lock
		_has_start = true;
		status = createWorkThread(pool, "sipecho", &worker_thread_proc, this, &_work_thread);
		createWorkThread(pool, "checkCall", &check_alive_thread, this, &_alive_thread);
		// register MQ callback functions that topic is IPC_REG_RESP and LIVE_MEDIA_STREAM
		// IPC_REG_RESP that datarmine whether IPC register request is logical.
		// LIVE_MEDIA_STREAM is a live request messge sent from RestFulInf .
		MqInf::ins()->setCallbackObject(IPC_REG_RESP, this);
		MqInf::ins()->setCallbackObject(LIVE_MEDIA_STREAM, this);
		MqInf::ins()->setCallbackObject(LIVE_START_REQ, this);
		MqInf::ins()->setCallbackObject(LIVE_STOP_REQ, this);
		MqInf::ins()->setCallbackObject(PTZ_SEND_REQ, this);
	}
	return status;
}
/*
 * stop sip server
*/
void SipApp::stop()
{
	TRACE_FUNC("SipApp::stop()");
	if (_has_start) {
		_has_start = false;
		LockGuard guard(_stopLock);
		MqInf::ins()->removeCallbackObject(IPC_REG_RESP, this);
		MqInf::ins()->removeCallbackObject(LIVE_MEDIA_STREAM, this);
		MqInf::ins()->removeCallbackObject(LIVE_START_REQ, this);
		MqInf::ins()->removeCallbackObject(LIVE_STOP_REQ, this);
		MqInf::ins()->removeCallbackObject(PTZ_SEND_REQ, this);
		/*first wait thread exit*/
		if (_work_thread) {
			pj_thread_join(_work_thread);
			pj_thread_destroy(_work_thread);
			_work_thread = NULL;
		}
		if (_alive_thread) {
			pj_thread_join(_alive_thread);
			pj_thread_destroy(_alive_thread);
			_alive_thread = NULL;
		}
		/*second stop all call*/
		hangupAll();
		pjsip_tsx_layer_destroy();
		if (_sip_tp) {
			pjsip_transport_shutdown(_sip_tp);
			pjsip_transport_destroy(_sip_tp);
			_sip_tp = NULL;
		}
		if (_sip_endpt) {
			pjsip_endpt_destroy(_sip_endpt);
			_sip_endpt = NULL;
		}
		if (_sipLock) {
			pj_lock_destroy(_sipLock);
			_sipLock = NULL;
		}
		if (_lockCall) {
			pj_lock_destroy(_lockCall);
			_lockCall = NULL;
		}
		if (_msgLock) {
			pj_lock_destroy(_msgLock);
			_msgLock = NULL;
		}
		if (_stopLock) {
			pj_lock_destroy(_stopLock);
			_stopLock = NULL;
		}
		if (_reqLock) {
			pj_lock_destroy(_reqLock);
			_reqLock = NULL;
		}
	}
}
/*
 * create work thread
*/
pj_status_t SipApp::createWorkThread(pj_pool_t* pool, const char* name, pj_thread_proc *proc, void *arg, pj_thread_t **thread)
{
	pj_status_t status = PJ_SUCCESS;
	status = pj_thread_create(pool, name, proc, arg, 0, 0, thread);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "create sip thread failed", status);
	}
	return status;
}

uint32_t SipApp::createSn()
{
	uint32_t val = 0;
	pj_lock_acquire(_sipLock);
	val = _snVal++;
	if (_snVal >= UINT_MAX) {
		_snVal = 0;
	}
	pj_lock_release(_sipLock);
	return val;
}

/*
 * hangup all IPC call 
*/
void SipApp::hangupAll(void)
{
	TRACE_FUNC("SipApp::hangupAll()");
	while(_lstCall.size() > 0) {
		CallList::iterator it = _lstCall.begin();
		IpcCall * ipcCall = *it;
		if (ipcCall) {
			call_t *call = ipcCall->getCall();
			if (call->inv) {
				pjsip_inv_terminate(call->inv, PJSIP_SC_DECLINE, true);
			}
			/*
			if (call->inv && call->inv->state <= PJSIP_INV_STATE_CONFIRMED) {
				pj_status_t status;
				pjsip_tx_data *tdata;
				status = pjsip_inv_end_session(call->inv, PJSIP_SC_BUSY_HERE, NULL, &tdata);
				if (status == PJ_SUCCESS && tdata) {
					pjsip_inv_send_msg(call->inv, tdata);
				}
			}
			reportIpcStatus(ipcCall->getIpcAddr(), ipcCall->getIpcPort(), 0);
			*/
			for(int i = 0; i < 3; i++)
				MqInf::ins()->loopMqMsg();
		}
	}
	_lstCall.clear();
}

/*
 * handle work thread event by SipApp method
*/
int SipApp::handle_work_thread_event()
{
	TRACE_FUNC("SipApp::handle_work_thread_event()");
	while (_has_start) {
		pj_time_val interval = { 0, 200 };
		pjsip_endpt_handle_events(_sip_endpt, &interval);
	}
	return 0;
}
int SipApp::handleAliveThread()
{
	while (_has_start) {
		for (CallList::iterator it = _lstCall.begin(); it != _lstCall.end(); ++it) {
			IpcCall * ipcCall = *it;
			if (ipcCall) {
				call_t *call = ipcCall->getCall();
				pj_time_val t;
				pj_gettimeofday(&t);
				PJ_TIME_VAL_SUB(t, call->connect_time);
				int difft = PJ_TIME_VAL_MSEC(t);
				int timeOut = ConfigMgr::ins()->getTimeout();
				if (timeOut < 0) {
					timeOut = 120;
				}
				if (difft > (timeOut * 1000)) {
					reportIpcStatus(ipcCall->getIpcAddr(), ipcCall->getIpcPort(), 0);
					pjsip_inv_terminate(call->inv, PJSIP_SC_DECLINE, true);
					break;
				} else {
					long diffTm = ipcCall->getInActivateTmLen();
					/*inactivate time greater than 30 sec */
					if (diffTm > (timeOut * 1000)) {
						if (call->inv) {
							pjsip_inv_terminate(call->inv, PJSIP_SC_DECLINE, true);
							break;
						}
					}
				}
			}
		}
		pj_thread_sleep(300);
	}
	return 0;
}
/*
 * handle incoming request event
 * when IPC push stream into sip server the function will been called at first.
*/
pj_bool_t SipApp::onRxRequest(pjsip_rx_data *rdata)
{
	if (!_has_start) {
		return PJ_TRUE;
	}
	SIP_LOG(SIP_INFO, (THIS_FILE, "onRxRequest %.*s from %s",
		static_cast<int>(rdata->msg_info.msg->line.req.method.name.slen),
		rdata->msg_info.msg->line.req.method.name.ptr,
		rdata->pkt_info.src_name));
	/*request lock required*/
	LockGuard guard(_reqLock);
	char poolBuf[1024];
	memset(poolBuf, 0, sizeof(poolBuf));
	pj_pool_t* pool = pj_pool_create_on_buf("onRxRequest", poolBuf, sizeof(poolBuf));
	// register request event
	if (rdata->msg_info.msg->line.req.method.id == PJSIP_REGISTER_METHOD) {
		handleRegisterRequest(pool, rdata);
		// pjsip_endpt_respond(_sip_endpt, &app_module, rdata, 200, NULL, &hdr_list, NULL, NULL);
	}else if (rdata->msg_info.msg->line.req.method.id == PJSIP_OTHER_METHOD) {// receive other message which id is PJSIP_OTHER_METHOD
		handleOtherMessageRequest(pool, rdata);
	} else if (rdata->msg_info.msg->line.req.method.id == PJSIP_INVITE_METHOD) {
		pjsip_cseq_hdr* seq_hdr = reinterpret_cast<pjsip_cseq_hdr*>(pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_CSEQ, NULL));
	} else if (rdata->msg_info.msg->line.req.method.id == PJSIP_BYE_METHOD) {
		SIP_LOG(SIP_INFO, (THIS_FILE, "receive a request method:PJSIP_BYE_METHOD"));
	}
	return PJ_TRUE;
}

/*
 * handle rx response from IPC
*/
pj_bool_t SipApp::onRxResponse(pjsip_rx_data *rdata)
{
	SIP_LOG(SIP_INFO, (THIS_FILE, "onRxResponse %.*s from %s",
		static_cast<int>(rdata->msg_info.msg->line.req.method.name.slen),
		rdata->msg_info.msg->line.req.method.name.ptr,
		rdata->pkt_info.src_name));
	if (rdata->msg_info.msg->line.req.method.id == PJSIP_BYE_METHOD) {
		SIP_LOG(SIP_INFO, (THIS_FILE, "receive a request method:PJSIP_BYE_METHOD"));
	}
	return PJ_FALSE;
}
/*
 * Tx response
*/
pj_bool_t SipApp::onTxResponse(pjsip_tx_data *tdata)
{
	SIP_LOG(SIP_INFO, (THIS_FILE, "onTxResponse %.*s from %s",
		static_cast<int>(tdata->msg->line.req.method.name.slen),
		tdata->msg->line.req.method.name.ptr,
		tdata->tp_info.dst_name));
	if (tdata->msg->line.req.method.id == PJSIP_INVITE_METHOD) {
		SIP_LOG(SIP_INFO, (THIS_FILE, "receive tx response method:PJSIP_INVITE_METHOD, status:%d", tdata->msg->line.status.code));
	}
	//pjsip_tx_data_dec_ref(tdata);
	return PJ_FALSE;
}
void SipApp::handleRegisterRequest(pj_pool_t* pool, pjsip_rx_data* rdata)
{
	pjsip_tx_data *tdata = NULL;
	pjsip_expires_hdr * expire_hdr = NULL;
	pjsip_authorization_hdr* auth_hdr = NULL;
	pjsip_msg *msg;
	int expires = -1;
	char val[255];
	char ipcIp[32];
	pjsip_response_addr addr;
	pjsip_get_response_addr(pool, rdata, &addr);
	// get ipc address
	memset(val, 0, sizeof(val));
	memset(ipcIp, 0, sizeof(ipcIp));
	memcpy(ipcIp, addr.dst_host.addr.host.ptr, addr.dst_host.addr.host.slen);
	int ipcPort = addr.dst_host.addr.port;

	msg = rdata->msg_info.msg;
	expire_hdr = reinterpret_cast<pjsip_expires_hdr*>(pjsip_msg_find_hdr(msg, PJSIP_H_EXPIRES, NULL));
	auth_hdr = reinterpret_cast<pjsip_authorization_hdr*>(pjsip_msg_find_hdr(msg, PJSIP_H_AUTHORIZATION, NULL));
	if (expire_hdr) {
		expires = expire_hdr->ivalue;
		/* register request */
		if (expires > 0) {
			if (auth_hdr) {
				handleAuthEvent(rdata, auth_hdr);
			} else {
				IpcCall * obj = findCallByAddrAndPort(ipcIp, ipcPort);
				if (!obj) {
					if (ConfigMgr::ins()->getRegAuth()) {
						pjsip_endpt_create_response(_sip_endpt, rdata, PJSIP_SC_UNAUTHORIZED, NULL, &tdata);
						responseToIpc(tdata, &addr, AuthHead);
					} else {
						pjsip_endpt_create_response(_sip_endpt, rdata, PJSIP_SC_OK, NULL, &tdata);
						responseToIpc(tdata, &addr, DateHead);
					}
				} else {
					pjsip_endpt_create_response(_sip_endpt, rdata, PJSIP_SC_OK, NULL, &tdata);
					responseToIpc(tdata, &addr, DateHead);
				}
			}
		} else {
			/* unregister request */
			pjsip_endpt_create_response(_sip_endpt, rdata, PJSIP_SC_OK, NULL, &tdata);
			responseToIpc(tdata, &addr, DateHead);
		}
	}
}

void SipApp::handleOtherMessageRequest(pj_pool_t* pool, pjsip_rx_data* rdata)
{
	char val[255];
	char ipcIp[32];
	pjsip_tx_data *tdata = NULL;
	pjsip_response_addr addr;
	pjsip_get_response_addr(pool, rdata, &addr);
	// get ipc address
	memset(val, 0, sizeof(val));
	memset(ipcIp, 0, sizeof(ipcIp));
	memcpy(ipcIp, addr.dst_host.addr.host.ptr, addr.dst_host.addr.host.slen);
	int ipcPort = addr.dst_host.addr.port;
	pjsip_cseq_hdr* seq_hdr = reinterpret_cast<pjsip_cseq_hdr*>(pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_CSEQ, NULL));
	if (seq_hdr) {
		int res = pj_strncmp2(&(seq_hdr->method.name), "MESSAGE", seq_hdr->method.name.slen);
		if (res == 0) {
			pj_xml_node* xmlRoot = pj_xml_parse(pool, static_cast<char*>((rdata->msg_info.msg->body->data)), rdata->msg_info.msg->body->len);
			if (xmlRoot) {
				pj_str_t cmdName = pj_str(const_cast<char*>("CmdType"));
				pj_xml_node* cmdNode = pj_xml_find_node(xmlRoot, &cmdName);
				pj_str_t nodeName = pj_str(const_cast<char*>("DeviceId"));
				pj_xml_node* idNode = pj_xml_find_node(xmlRoot, &nodeName);
				if (idNode) {
					std::string deviceId;
					memset(val, 0, sizeof(val));
					memcpy(val, idNode->content.ptr, idNode->content.slen);
					deviceId = val;
					// at first send a response with 200 OK status.
					pjsip_endpt_create_response(_sip_endpt, rdata, PJSIP_SC_OK, NULL, &tdata);
					responseToIpc(tdata, &addr, NoHead);
					// if IpcCall's object has been created and exited in list then do nothing else new it.
					IpcCall * obj = findCallByAddrAndPort(ipcIp, ipcPort);
					if (!obj) {
						/*new a object to handle RTP from IPC*/
						if (ConfigMgr::ins()->getUseTcp()) {
							obj = new IpcWithTcp(deviceId.c_str(), ipcIp, ipcPort, createNextRtpPort(), createSn());
						} else {
							obj = new IpcWithPjMedia(deviceId.c_str(), ipcIp, ipcPort, createNextRtpPort(), createSn());
						}
						if (obj->start(this->_cp) == -1) {
							SIP_LOG(SIP_ERROR, (THIS_FILE, "start ipc call failed"));
							delete obj;
						} else {
							// send sdp invite IPC
							if (sendSdpToIpc2(rdata, obj) == PJ_FALSE) {
								obj->stop();
								delete obj;
							} else {
								pj_gettimeofday(&(obj->getCall()->connect_time));
								pj_lock_acquire(_lockCall);
								_lstCall.push_back(obj);
								pj_lock_release(_lockCall);
								reportIpcStatus(obj->getIpcAddr(), obj->getIpcPort(), 1);
								SIP_LOG(SIP_INFO, (THIS_FILE, "IPC list size: %d", _lstCall.size()));
							}
						}
					} else {
						pj_gettimeofday(&(obj->getCall()->connect_time));
						reportIpcStatus(obj->getIpcAddr(), obj->getIpcPort(), 1);
					}
				}
			}
		}
	}
}
/*
 * handle Authorization message from IPC
*/
void SipApp::handleAuthEvent(pjsip_rx_data *rdata, pjsip_authorization_hdr* authHdr)
{
	if (!rdata || !authHdr) {
		return;
	}
	char* poolBuf = new char[512];
	memset(poolBuf, 0, 512);
	pj_pool_t* pool = pj_pool_create_on_buf("Auth", poolBuf, 512);
	RegRequest obj;
	char val[255];
	char sipId[64];
	memset(sipId, 0, sizeof(sipId));
	SIP_LOG(SIP_INFO, (THIS_FILE, "srcname=%s,port=%d ", rdata->pkt_info.src_name, rdata->pkt_info.src_port));
	SIP_LOG(SIP_INFO, (THIS_FILE,
		"username=%s, uri=%s, passval=%s,realm=%s",
		authHdr->credential.digest.username.ptr,
		authHdr->credential.digest.uri.ptr,
		authHdr->credential.digest.nonce.ptr,
		authHdr->credential.common.realm.ptr));
	memset(val, 0, sizeof(val));
	pj_ansi_snprintf(val, sizeof(val), "%s:%d", rdata->pkt_info.src_name, rdata->pkt_info.src_port);
	obj.setIpcAddr(val);
	memset(val, 0, sizeof(val));
	memcpy(val, authHdr->credential.digest.nonce.ptr, authHdr->credential.digest.nonce.slen);
	obj.setPassword(val);
	memset(val, 0, sizeof(val));
	memcpy(val, authHdr->credential.digest.username.ptr, authHdr->credential.digest.username.slen);
	obj.setUserName(val);
	memset(val, 0, sizeof(val));
	memcpy(val, authHdr->credential.digest.realm.ptr, authHdr->credential.digest.realm.slen);
	obj.setRealm(val);
	memset(val, 0, sizeof(val));
	memcpy(val, authHdr->credential.digest.response.ptr, authHdr->credential.digest.response.slen);
	obj.setResponse(val);
	memset(val, 0, sizeof(val));
	memcpy(val, authHdr->credential.digest.uri.ptr, authHdr->credential.digest.uri.slen);
	obj.setUrl(val);
	char * idxStr = strstr(val, "@");
	char * idxStr2 = strstr(val, "sip:");
	if (idxStr2) {
		idxStr2 += strlen("sip:");
	} else {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "cann't find prefix 'sip' in uri"));
		delete[] poolBuf;
		return;
	}
	if (idxStr) {
		memcpy(sipId, idxStr2, idxStr - idxStr2);
	} else {
		pj_ansi_strncpy(sipId, obj.getUserName(), sizeof(sipId));
	}
	obj.setSipId(sipId);
	_sipId = sipId;
	uint32_t sn = createSn();
	response_data_t * rspData = static_cast<response_data_t*>(pj_pool_alloc(pool, sizeof(response_data_t)));
	rspData->buf = poolBuf;
	pjsip_endpt_create_response(_sip_endpt, rdata, PJSIP_SC_OK, NULL, &(rspData->_data));
	pjsip_get_response_addr(pool, rdata, &(rspData->_addr));
	pj_lock_acquire(_sipLock);
	_mapRspData.insert(std::make_pair(sn, rspData));
	pj_lock_release(_sipLock);
	obj.setSn(sn);
	MqInf::ins()->publishMsg(IPC_REG_REQ, &obj);
}
/*
 * response to ipc for register result
*/
void SipApp::responseToIpc(pjsip_tx_data* tdata, pjsip_response_addr *addr, HeadType headType)
{
	char poolBuf[512];
	memset(poolBuf, 0, 512);
	pj_pool_t *pool = pj_pool_create_on_buf("responseToIpc", poolBuf, sizeof(poolBuf));
	time_t t;
	time(&t);
	struct tm* ptm = localtime(&t);
	char chTm[64];
	if (ptm) {
		snprintf(chTm, 64, "%04d-%02d-%02d %02d:%02d:%02d", ptm->tm_year + 1900, ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	}
	pj_status_t status;
	pj_str_t key;
	pj_str_t c;
	pjsip_hdr *hdr;

	switch (headType)
	{
	case SipApp::DateHead:
		key = pj_str(const_cast<char*>("Date"));
		hdr = reinterpret_cast<pjsip_hdr*>(pjsip_date_hdr_create(pool, &key, pj_cstr(&c, chTm)));
		pjsip_msg_add_hdr(tdata->msg, hdr);
		break;
	case SipApp::AuthHead:
		{
			struct pjsip_auth_srv auth_srv;
			pj_str_t realm = pj_str(const_cast<char*>("localhost"));
			pjsip_auth_srv_init(pool, &auth_srv, &realm, pjsip_auth_lookup, 0);
			pjsip_auth_srv_challenge(&auth_srv, NULL, NULL, NULL, PJ_FALSE, tdata);
		}
		break;
	default:
		break;
	}
	status = pjsip_endpt_send_response(_sip_endpt, addr, tdata, NULL, NULL);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "sip send response failed", status);
	} 
}

/*
 * Callback function from MQ
 * topic has registed in start() function
 * will handle message include IPC_REG_RESP, LIVE_MEDIA_STREAM LIVE_START_REQ LIVE_STOP_REQ
*/
void SipApp::onMsgCallback(const char* topic, MqObject *obj)
{
	if (obj == NULL) {
		return;
	}
	if (strcmp(topic, IPC_REG_RESP) == 0) {
		ResponseObj * repObj = static_cast<ResponseObj*>(obj);
		uint32_t sn = repObj->getSn();
		int code = repObj->getCode();
		pj_lock_acquire(_sipLock);
		RspDataMap::iterator it = _mapRspData.find(sn);
		if (it != _mapRspData.end()) {
			response_data_t* rspData = it->second;
			if (code == 200) {
				rspData->_data->msg->line.status.code = PJSIP_SC_OK;
			}else {
				rspData->_data->msg->line.status.code = PJSIP_SC_UNAUTHORIZED;
			}
			responseToIpc(rspData->_data, &(rspData->_addr), DateHead);
			if (rspData->buf) {
				delete[] rspData->buf;
			}
			_mapRspData.erase(it);
		}
		pj_lock_release(_sipLock);
	} else if (strcmp(topic, LIVE_MEDIA_STREAM) == 0) {
		LiveStreamResp * respObj = reinterpret_cast<LiveStreamResp*>(obj);
	} else if (strcmp(topic, LIVE_START_REQ) == 0) {
		pj_lock_acquire(_msgLock);
		LiveMediaStart* startObj = reinterpret_cast<LiveMediaStart*>(obj);
		/*make a LiveRtpCli object and insert to stream-channel of StreamMgr*/
		LiveRtpCli* liveCli = makeLiveRtpClient(startObj);
		if (liveCli) {
			const char* fmt = StreamMgr::ins()->getFmtByIpcUrl(liveCli->getIpcAddr());
			/*begin to make a response to publisher*/
			LiveMediaStartResp resp;
			char localAddr[32];
			memset(localAddr, 0, sizeof(localAddr));
			memcpy(localAddr, _localAddr.ptr, _localAddr.slen);
			resp.setCliAddr(localAddr);
			resp.setIpcAddr(liveCli->getIpcAddr());
			resp.setFmt(fmt);
			resp.setStartPort(liveCli->getPort());
			resp.setEndPort(liveCli->getPort() + 1);
			MqInf::ins()->publishMsg(LIVE_START_RESP, &resp); // send LIVE_START_RESP to publisher
		}
		pj_lock_release(_msgLock);
	} else if (strcmp(topic, LIVE_STOP_REQ) == 0) {
		pj_lock_acquire(_msgLock);
		LiveStopReq* stopObj = reinterpret_cast<LiveStopReq*>(obj);
		/*when receive a live stop command we need remove the live object from StreamMgr*/
		removeLiveRtpClient(stopObj);
		pj_lock_release(_msgLock);
	} else if (strcmp(topic, PTZ_SEND_REQ) == 0) {
		/*PTZ command ctrl*/
		PTZCmd* cmdObj = reinterpret_cast<PTZCmd*>(obj);
		sendPTZ2Ipc(cmdObj);
	}
}
/*
 * return LiveRtpCli object if find it in std::map by key ipcAddress
*/
LiveRtpCli* SipApp::makeLiveRtpClient(LiveMediaStart* obj)
{
	std::string ipcUrl = obj->getIpcAddr();
	StreamMgr::ins()->removeLiveCliByIpcUrl(ipcUrl.c_str());
	LiveRtpCli* liveCli = new LiveRtpCli();
	int port = createNextRtpPort();
	if (liveCli->open(port, ipcUrl.c_str(), obj->getSvcAddr(), obj->getStartPort()) == -1) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "open liveRtpCli failed\n"));
		delete liveCli;
		liveCli = NULL;
	} else {
		StreamMgr::ins()->addIpcChannelByIpcUrl(ipcUrl.c_str(), true, liveCli);
	}
	return liveCli;
}
/*
 *  called when receive MQ message which topic is LIVE_STOP_REQ
 * In the function call StreamMgr to set LiveRtpCli to null 
*/
void SipApp::removeLiveRtpClient(LiveStopReq* obj)
{
	if (obj == NULL) {
		return;
	}
	StreamMgr::ins()->removeLiveCliByIpcUrl(obj->getIpcAddr());
}

unsigned int SipApp::createNextRtpPort()
{
	pj_lock_acquire(_sipLock);
	unsigned int port = _rtpNextPort;
	_rtpNextPort += ConfigMgr::ins()->getRtpPortStep();
	pj_lock_release(_sipLock);
	return port;
}

pj_bool_t SipApp::sendSdpToIpc2(pjsip_rx_data* rdata, IpcCall* obj)
{
	if (obj == NULL) {
		return PJ_FALSE;
	}
	pjsip_dialog *dlg;
	pjsip_tx_data *tdata;
	//pjmedia_sdp_session* local_sdp = NULL;
	pj_status_t status = PJ_SUCCESS;
	call_t* call = obj->getCall();
	char val[255];
	pj_pool_t* pool = obj->getPool();
	if (ConfigMgr::ins()->getUseTcp()) {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=TCP>", obj->getDeviceId().c_str(), rdata->pkt_info.src_name, rdata->pkt_info.src_port);
	} else {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d>", obj->getDeviceId().c_str(), rdata->pkt_info.src_name, rdata->pkt_info.src_port);
	}
	pj_str_t remoteUri = pj_str(val);
	if (!call->inv) {
		/* Create UAS dialog */
		status = pjsip_dlg_create_uac(pjsip_ua_instance(), &_localUri, &_localContact, &remoteUri, &remoteUri, &dlg);
		if (status != PJ_SUCCESS) {
			ERROR_LOG(THIS_FILE, "pjsip_dlg_create_uac failed", status);
			return PJ_FALSE;
		}
		/* Create UAS invite session */
		status = pjsip_inv_create_uac(dlg, 0, 0, &call->inv);
		if (status != PJ_SUCCESS) {
			pjsip_dlg_terminate(dlg);
			return PJ_FALSE;
		}
	}
	status = pjsip_inv_invite(call->inv, &tdata);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "pjsip_inv_invite failed", status);
		return PJ_FALSE;
	}
	call->inv->mod_data[app_module.id] = call; // put call pointer into mod_data of inv;
	std::string sdp = createSdp(obj);
	pj_str_t text = pj_str((char*)sdp.c_str());
	pjsip_media_type type;
	type.type = pj_str(const_cast<char*>("application"));
	type.subtype = pj_str(const_cast<char*>("sdp"));
	tdata->msg->body = pjsip_msg_body_create(pool, &type.type, &type.subtype, &text);
	auto hName = pj_str(const_cast<char*>("Subject"));
	sprintf(val, "%s : 1, %s : 1", _sipId.c_str(), _sipId.c_str());
	auto hValue = pj_str(val);
	auto hdr = pjsip_generic_string_hdr_create(pool, &hName, &hValue);
	pjsip_msg_add_hdr(tdata->msg, reinterpret_cast<pjsip_hdr*>(hdr));
	status = pjsip_inv_send_msg(call->inv, tdata);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "pjsip_inv_send_msg failed", status);
		pjsip_inv_terminate(call->inv, PJSIP_SC_DECLINE, true);
		return PJ_FALSE;
	}
	return PJ_TRUE;
}

std::string SipApp::createSdp(IpcCall* pCall)
{
	char str[1024] ;
	char videoInfo[64];
	char aInfo[128];
	char transport[32];
	memset(str, 0, sizeof(str));
	memset(videoInfo, 0, sizeof(videoInfo));
	memset(aInfo, 0, sizeof(aInfo));
	for (std::vector<codec>::iterator it = _vecCodec.begin(); it != _vecCodec.end(); ++it) {
		pj_ansi_snprintf(videoInfo + strlen(videoInfo), sizeof(videoInfo) - strlen(videoInfo) - 1, "%d ", it->pt);
		pj_ansi_snprintf(aInfo + strlen(aInfo),
			sizeof(aInfo) - strlen(aInfo) - 1,
			"a=rtpmap:%d %s/%d\r\n",
			it->pt,
			it->name,
			it->clock_rate);
	}
	int frame = ConfigMgr::ins()->getFrame();
	bool useTcp = ConfigMgr::ins()->getUseTcp();
	if (useTcp) {
		pj_ansi_snprintf(transport, sizeof(transport) - 1, "TCP/RTP/AVP");
		pj_ansi_snprintf(aInfo + strlen(aInfo), sizeof(aInfo) - strlen(aInfo) - 1, "a=connection:new\r\n");
	} else {
		pj_ansi_snprintf(transport, sizeof(transport) - 1, "RTP/AVP");
	}
	pj_ansi_snprintf(str,
		sizeof(str) - 1,
		"v=0\r\n"
		"o=%s 0 0 IN IP4 %s\r\n"
		"s=Play\r\n"
		"c=IN IP4 %s\r\n"
		"t=0 0\r\n"
		"m=video %d %s %s\r\n"
		"a=framerate:%d\r\n"
		"a=recvonly\r\n"
		"%s\r\n",
		pCall->getDeviceId().c_str(),
		pCall->getRecvAddr().c_str(),
		pCall->getRecvAddr().c_str(),
		pCall->getLocalRtpPort(),
		transport,
		videoInfo,
		frame,
		aInfo);
	return std::string(str);
}

/*
 * destroy a IPC stream call
*/
void SipApp::destroyCallMedia(unsigned int index)
{
	TRACE_FUNC("SipApp::destroyCallMedia()");
	pj_lock_acquire(_lockCall);
	for (CallList::iterator it = _lstCall.begin(); it != _lstCall.end(); ++it) {
		IpcCall* ipcCall = *it;
		if (ipcCall && ipcCall->getCall()->index == index) {
			reportIpcStatus(ipcCall->getIpcAddr(), ipcCall->getIpcPort(), 0);
			ipcCall->stop();
			_lstCall.erase(it++);
			delete ipcCall;
			break;
		}
	}
	pj_lock_release(_lockCall);
	if(_lstCall.size() == 0) {
		MallocExtension::instance()->ReleaseFreeMemory();
	}
}
/*
 *  find a object which class is IpcCall
 *  ipcUri: uri of IPC ip:port
*/
IpcCall* SipApp::findCallByUri(const char* ipcUri)
{
	IpcCall* call = NULL;
	pj_lock_acquire(_lockCall);
	for (CallList::iterator it = _lstCall.begin(); it != _lstCall.end(); ++it) {
		IpcCall* ipcCall = *it;
		char uri[64];
		memset(uri, 0, sizeof(uri));
		pj_ansi_snprintf(uri, sizeof(uri) - 1, "%s:%d", ipcCall->getIpcAddr(), ipcCall->getIpcPort());
		if (ipcCall && strcmp(uri, ipcUri) == 0) {
			call = ipcCall;
			break;
		}
	}
	pj_lock_release(_lockCall);
	return call;
}

IpcCall* SipApp::findCallByAddrAndPort(const char* ipcAddr, int port)
{
	IpcCall* call = NULL;
	pj_lock_acquire(_lockCall);
	for (CallList::iterator it = _lstCall.begin(); it != _lstCall.end(); ++it) {
		IpcCall* ipcCall = *it;
		if (ipcCall && strcmp(ipcCall->getIpcAddr(), ipcAddr) == 0 && ipcCall->getIpcPort() == port) {
			call = ipcCall;
			break;
		}
	}
	pj_lock_release(_lockCall);
	return call;
}
/*
*  find a object which class is IpcCall
*  index: call_t's index
*/
IpcCall* SipApp::findCallByIndex(unsigned int index)
{
	IpcCall* call = NULL;
	LockGuard guard(_lockCall);
	for (CallList::iterator it = _lstCall.begin(); it != _lstCall.end(); ++it) {
		IpcCall* ipcCall = *it;
		if (ipcCall && ipcCall->getCall()->index == index) {
			call = ipcCall;
			break;
		}
	}
	return call;
}

void SipApp::onStateChanged(pjsip_inv_session *inv, pjsip_event *e)
{
	TRACE_FUNC("SipApp::onStateChanged()");
	// stop Lock required because of hungupAll() maybe call first
	struct call_t *call = (struct call_t*)inv->mod_data[app_module.id];
	if (!call) {
		goto exit;
	}
	SIP_LOG(SIP_INFO, (THIS_FILE, "onStateChange, state=%d", inv->state));
	if (inv->state == PJSIP_INV_STATE_DISCONNECTED) {
		SIP_LOG(SIP_INFO, (THIS_FILE, "Call #%d disconnected. Reason=%d (%.*s)",
			call->index,
			inv->cause,
			static_cast<int>(inv->cause_text.slen),
			inv->cause_text.ptr));
		destroyCallMedia(call->index);
	} else if (inv->state == PJSIP_INV_STATE_CONFIRMED) {
		pj_time_val t;
		pj_gettimeofday(&call->connect_time);
		if (call->response_time.sec == 0) {
			call->response_time = call->connect_time;
		}
		t = call->connect_time;
		PJ_TIME_VAL_SUB(t, call->start_time);
		SIP_LOG(SIP_INFO, (THIS_FILE, "Call #%d connected in %d ms", call->index, PJ_TIME_VAL_MSEC(t)));
		
	} else if (inv->state == PJSIP_INV_STATE_CALLING) {// after invite is sent
		SIP_LOG(SIP_INFO, (THIS_FILE, "Call #%d is calling. Reason=%d (%.*s)",
			call->index,
			inv->cause,
			static_cast<int>(inv->cause_text.slen),
			inv->cause_text.ptr));
	}else if (inv->state == PJSIP_INV_STATE_EARLY || inv->state == PJSIP_INV_STATE_CONNECTING) {
		if (call->response_time.sec == 0)
			pj_gettimeofday(&call->response_time);
	}
exit:
	return;
}
/*
 * callback when new session is coming
*/
void SipApp::onNewSession(pjsip_inv_session *inv, pjsip_event *e)
{
	TRACE_FUNC("SipApp::onNewSession()");
}
/*
 * callback when status is media update
 * the status tell us that media stream will come
*/
void SipApp::onMediaUpdate(pjsip_inv_session *inv, pj_status_t status)
{
	TRACE_FUNC("SipApp::onMediaUpdate()");
	struct call_t * call = (struct call_t*)inv->mod_data[app_module.id];
	pj_lock_acquire(_lockCall);
	for (CallList::iterator it = _lstCall.begin(); it != _lstCall.end(); ++it) {
		IpcCall* ipcCall = *it;
		if (ipcCall && ipcCall->getCall()->index == call->index) {
			ipcCall->onMediaUpdate(inv, status);
			break;
		}
	}
	pj_lock_release(_lockCall);

	/*if need force key-frame then call it in here*/
	if (ConfigMgr::ins()->getForceKeyFrame()) {
		sendForceKeyFrame(call);
	}
}

/*
 * send a answer for the offer from IPC
*/
void SipApp::onRxOffer(pjsip_inv_session *inv, const pjmedia_sdp_session *offer)
{
	TRACE_FUNC("SipApp::onRxOffer()");
	char buf[4096];
	memset(buf, 0, sizeof(buf));
	pj_pool_t * pool = pj_pool_create_on_buf("onRxOffer", buf, sizeof(buf));
	call_t *call = static_cast<call_t*>(inv->mod_data[app_module.id]);
	if (call) {
		pjmedia_sdp_session* answer = createAnswer(pool, offer);
		pjsip_inv_set_sdp_answer(inv, answer);
		/*after send answer then start IPC media*/
		char ipcAddr[32];
		memset(ipcAddr, 0, sizeof(ipcAddr));
		memcpy(ipcAddr, offer->conn->addr.ptr, offer->conn->addr.slen);
		int remoteRtpPort = 0;
		int frame = ConfigMgr::ins()->getFrame();
		char fmt[32];
		char val[32];
		for (int i = 0; i < offer->media_count; i++) {
			pjmedia_sdp_media *m = offer->media[i];
			remoteRtpPort = m->desc.port;
			for (int n = 0; n < m->attr_count; n++) {
				if (pj_ansi_strncmp(m->attr[n]->name.ptr, "framerate", m->attr[n]->name.slen) == 0) {
					pj_ansi_strncpy(val, m->attr[n]->value.ptr, m->attr[n]->value.slen);
					val[m->attr[n]->value.slen] = 0;
					frame = atoi(val);
				}
				if (pj_ansi_strncmp(m->attr[n]->name.ptr, "rtpmap", m->attr[n]->name.slen) == 0) {
					pj_ansi_strncpy(val, m->attr[n]->value.ptr, m->attr[n]->value.slen);
					val[m->attr[n]->value.slen] = 0;
					try {
						MyRegEx reg("([a-zA-Z0-9]+)/([0-9]+)");
						MyMatch m;
						if (MyRegSearch(val, m, reg) && m.size() >= 3) {
							pj_ansi_strncpy(fmt, m[1].str().c_str(), 30);
							fmt[m[1].str().length()] = 0;
						}
					} catch (MyRegError &e) {

					}
				}
			}
		}
		IpcCall* ipcCall = findCallByIndex(call->index);
		if(ipcCall) {
			ipcCall->setSdpAttr(frame, remoteRtpPort, fmt);
		}
	}
}

pjmedia_sdp_session *SipApp::createAnswer(pj_pool_t* pool, const pjmedia_sdp_session *offer)
{
	const char* dir_attrs[] = { "sendrecv", "sendonly", "recvonly", "inactive" };
	const char *ice_attrs[] = { "ice-pwd", "ice-ufrag", "candidate" };
	pjmedia_sdp_attr *sess_dir_attr = NULL;
	pjmedia_sdp_session *answer = pjmedia_sdp_session_clone(pool, offer);
	answer->name = pj_strdup3(pool, const_cast<char*>("sipapp"));
	sess_dir_attr = removeSdpAttrs(&answer->attr_count, answer->attr, PJ_ARRAY_SIZE(dir_attrs), dir_attrs);
	for (int i = 0; i < answer->media_count; i++) {
		pjmedia_sdp_media *m = answer->media[i];
		pjmedia_sdp_attr *m_dir_attr;
		pjmedia_sdp_attr *dir_attr;
		char *our_dir = NULL;
		pjmedia_sdp_conn *c;
		m_dir_attr = removeSdpAttrs(&m->attr_count, m->attr, PJ_ARRAY_SIZE(dir_attrs), dir_attrs);
		dir_attr = m_dir_attr ? m_dir_attr : sess_dir_attr;

		if (dir_attr) {
			if (pj_strcmp2(&dir_attr->name, const_cast<char*>("sendonly")) == 0)
				our_dir = const_cast<char*>("recvonly");
			else if (pj_strcmp2(&dir_attr->name, const_cast<char*>("inactive")) == 0)
				our_dir = const_cast<char*>("inactive");
			else if (pj_strcmp2(&dir_attr->name, const_cast<char*>("recvonly")) == 0)
				our_dir = const_cast<char*>("inactive");

			if (our_dir) {
				dir_attr = PJ_POOL_ZALLOC_T(pool, pjmedia_sdp_attr);
				dir_attr->name = pj_strdup3(pool, our_dir);
				m->attr[m->attr_count++] = dir_attr;
			}
		}
		removeSdpAttrs(&m->attr_count, m->attr, PJ_ARRAY_SIZE(ice_attrs), ice_attrs);
		c = m->conn ? m->conn : answer->conn;
		SIP_LOG(SIP_INFO, (THIS_FILE, "  Media %d, %.*s: %s <--> %.*s:%d",
			i, static_cast<int>(m->desc.media.slen), m->desc.media.ptr,
			(our_dir ? our_dir : const_cast<char*>("sendrecv")),
			static_cast<int>(c->addr.slen), c->addr.ptr, m->desc.port));
	}
	return answer;
}

pjmedia_sdp_attr * SipApp::removeSdpAttrs(unsigned *cnt, pjmedia_sdp_attr *attr[], unsigned cnt_attr_to_remove, const char* attr_to_remove[])
{
	pjmedia_sdp_attr *found_attr = NULL;
	int i;

	for (i = 0; i < static_cast<int>(*cnt); ++i) {
		unsigned j;
		for (j = 0; j<cnt_attr_to_remove; ++j) {
			if (pj_strcmp2(&attr[i]->name, attr_to_remove[j]) == 0) {
				if (!found_attr) found_attr = attr[i];
				pj_array_erase(attr, sizeof(attr[0]), *cnt, i);
				--(*cnt);
				--i;
				break;
			}
		}
	}

	return found_attr;
}

static void send_request_callback(pjsip_send_state *st, pj_ssize_t sent, pj_bool_t *cont)
{
	SipApp* app = reinterpret_cast<SipApp*>(st->token);
	if (app) {
		app->onSendRequestCallback(st, sent, cont);
	}
}
void SipApp::onSendRequestCallback(pjsip_send_state *st, pj_ssize_t sent, pj_bool_t *cont)
{
	pjsip_tx_data* tdata = st->tdata;
	char* destIp = tdata->tp_info.dst_name;
	int port = tdata->tp_info.dst_port;
	if (tdata->msg->line.req.method.id == PJSIP_OTHER_METHOD) {
		int cmp = pj_ansi_strncmp(tdata->msg->line.req.method.name.ptr, pjsip_message_method.name.ptr, pjsip_message_method.name.slen);
		if (cmp == 0) {
			pj_thread_sleep(2000);
			sendPTZ2IpcStop(destIp, port);
		}
	}
}
/*
 * send PTZ command to IPC
*/
void SipApp::sendPTZ2Ipc(PTZCmd* obj)
{
	TRACE_FUNC("SipApp::sendPTZ2Ipc()");
	char poolBuf[1024];
	memset(poolBuf, 0, sizeof(poolBuf));
	pj_pool_t * pool = pj_pool_create_on_buf("sendPTZ2Ipc", poolBuf, sizeof(poolBuf));
	const char* ipcAddr = obj->getIpcAddr();
	int cmd = obj->getCmd();
	
	IpcCall* ipcObj = findCallByUri(ipcAddr);
	if (ipcObj == NULL) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "sendPTZ2Ipc(),Cann't find the IPC that address is :%s", ipcAddr));
		return;
	}
	const char* channelId = ConfigMgr::ins()->getChannelId();
	pj_status_t status = PJ_SUCCESS;
	pjsip_tx_data *tdata;
	pjsip_media_type media_type;
	pj_str_t target, from, callId;
	const char* ipcIp = ipcObj->getIpcAddr();
	int ipcPort = ipcObj->getIpcPort();
	char val[255];
	if (ConfigMgr::ins()->getUseTcp()) {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=TCP>", channelId, ipcIp, ipcPort);
	} else {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=udp>", channelId, ipcIp, ipcPort);
	}
	target = pj_str(val);
	pj_strassign(&from, &_localUri);
	callId = pj_str((char*)_sipId.c_str());
	int sn = createSn();
	status = pjsip_endpt_create_request(_sip_endpt, &pjsip_message_method, &target, &from, &target, &from, &callId, sn, NULL, &tdata);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "create PTZ message request error", status);
		return;
	}
	char body[512];
	char cmdStr[64];
	int sum = 0;
	unsigned char chCmd = cmd;
	unsigned char checkSum = 0;
	pj_str_t msgBody;
	unsigned char speed = 0xA1;
	switch (chCmd) {
	case 0x01:
	case 0x02:
		pj_ansi_snprintf(cmdStr, sizeof(cmdStr), "%02X%02X0000", chCmd, speed);
		sum = 0xA5 + 0x0F + 0x01 + cmd + speed + 0x00 + 0x00;
		break;
	case 0x04:
	case 0x08:
		pj_ansi_snprintf(cmdStr, sizeof(cmdStr), "%02X00%02X00", chCmd, speed);
		sum = 0xA5 + 0x0F + 0x01 + cmd + 0x00 + speed + 0x00;
		break;
	case 0x10:
	case 0x20:
		pj_ansi_snprintf(cmdStr, sizeof(cmdStr), "%02X0000A0", chCmd);
		sum = 0xA5 + 0x0F + 0x01 + cmd + 0x00 + 0x00 + 0xA0;
		break;
	}
	checkSum = sum % 256;
	pj_ansi_sprintf(val, "%02X", checkSum);
	pj_ansi_snprintf(body, sizeof(body),
		"<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<PTZCmd>A50F01%s%s</PTZCmd>\r\n"
		"</Control>\r\n",
		sn,
		channelId,
		cmdStr,
		val);
	
	media_type.type = pj_str(const_cast<char*>("application"));
	media_type.subtype = pj_str(const_cast<char*>("MANSCDP+xml"));
	msgBody = pj_str(body);
	tdata->msg->body = pjsip_msg_body_create(pool, &media_type.type, &media_type.subtype, &msgBody);
	status = pjsip_endpt_send_request_stateless(_sip_endpt, tdata, this, &send_request_callback);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "send PTZ message request failed", status);
	}
}
void SipApp::sendPTZ2IpcStop(const char* ipcAddr, int port)
{
	IpcCall* ipcObj = findCallByAddrAndPort(ipcAddr, port);
	if (ipcObj == NULL) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "sendPTZ2Ipc(),Cann't find the IPC that address is :%s", ipcAddr));
		return;
	}
	char poolBuf[1024];
	memset(poolBuf, 0, sizeof(poolBuf));
	pj_pool_t * pool = pj_pool_create_on_buf("sendPTZ2Ipc", poolBuf, sizeof(poolBuf));
	const char* channelId = ConfigMgr::ins()->getChannelId();
	pj_status_t status = PJ_SUCCESS;
	pjsip_tx_data *tdata;
	pjsip_media_type media_type;
	pj_str_t target, from, callId;
	const char* ipcIp = ipcObj->getIpcAddr();
	int ipcPort = ipcObj->getIpcPort();
	char val[255];
	if (ConfigMgr::ins()->getUseTcp()) {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=TCP>", channelId, ipcIp, ipcPort);
	} else {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=udp>", channelId, ipcIp, ipcPort);
	}
	target = pj_str(val);
	pj_strassign(&from, &_localUri);
	callId = pj_str((char*)_sipId.c_str());
	int sn = createSn();
	status = pjsip_endpt_create_request(_sip_endpt, &pjsip_message_method, &target, &from, &target, &from, &callId, sn, NULL, &tdata);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "create PTZ message request error", status);
		return;
	}

	char body[512];
	pj_str_t msgBody;
	pj_ansi_snprintf(body, sizeof(body),
		"<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<PTZCmd>A50F0100000000B5</PTZCmd>\r\n"
		"</Control>\r\n",
		sn,
		channelId);
	media_type.type = pj_str(const_cast<char*>("application"));
	media_type.subtype = pj_str(const_cast<char*>("MANSCDP+xml"));
	msgBody = pj_str(body);
	tdata->msg->body = pjsip_msg_body_create(pool, &media_type.type, &media_type.subtype, &msgBody);
	status = pjsip_endpt_send_request_stateless(_sip_endpt, tdata, NULL, NULL);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "send PTZ message request failed", status);
	}
}

void SipApp::sendForceKeyFrame(struct call_t* call)
{
	TRACE_FUNC("SipApp::sendForceKeyFrame()");
	if (call == NULL) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "the pointer of call is null, sendForceKeyFrame failed"));
		return;
	}
	IpcCall* ipcObj = findCallByIndex(call->index);
	if(ipcObj == NULL) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "sendForceKeyFrame(), ipcCall is NULL return"));
		return;
	}
	char poolBuf[1024];
	memset(poolBuf, 0, sizeof(poolBuf));
	pj_pool_t * pool = pj_pool_create_on_buf("sendForceKeyFrame", poolBuf, sizeof(poolBuf));
	const char* channelId = ConfigMgr::ins()->getChannelId();
	pj_status_t status = PJ_SUCCESS;
	pjsip_tx_data *tdata;
	pjsip_media_type media_type;
	char body[512];
	pj_str_t msgBody;
	pj_str_t target, from, callId;
	const char* ipcIp = ipcObj->getIpcAddr();
	int ipcPort = ipcObj->getIpcPort();
	char val[255];
	if (ConfigMgr::ins()->getUseTcp()) {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=TCP>", channelId, ipcIp, ipcPort);
	} else {
		pj_ansi_sprintf(val, "<sip:%s@%s:%d;transport=udp>", channelId, ipcIp, ipcPort);
	}
	target = pj_str(val);
	pj_strassign(&from, &_localUri);
	callId = pj_str((char*)_sipId.c_str());
	int sn = createSn();
	status = pjsip_endpt_create_request(_sip_endpt, &pjsip_message_method, &target, &from, &target, &from, &callId, sn, NULL, &tdata);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "create forceKeyFrame message request error", status);
		return;
	}
	pj_ansi_snprintf(body, sizeof(body),
		"<?xml version=1.0 ?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<IFameCmd>Send</IFameCmd>\r\n"
		"</Control>\r\n",
		sn,
		channelId);
	media_type.type = pj_str(const_cast<char*>("application"));
	media_type.subtype = pj_str(const_cast<char*>("MANSCDP+xml"));
	msgBody = pj_str(body);
	tdata->msg->body = pjsip_msg_body_create(pool, &media_type.type, &media_type.subtype, &msgBody);
	status = pjsip_endpt_send_request_stateless(_sip_endpt, tdata, NULL, NULL);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "send forceKeyFrame message request failed", status);
	}
}

void SipApp::reportIpcStatus(const char* ipcAddr, int port, int status)
{
	StatusRpt obj;
	char val[64];
	memset(val, 0, sizeof(val));
	pj_ansi_snprintf(val, sizeof(val) - 1, "%s:%d", ipcAddr, port);
	obj.setIpcAddr(val);
	obj.setStatus(status);
	MqInf::ins()->publishMsg(IPC_STATUS_RPT, &obj);
}
