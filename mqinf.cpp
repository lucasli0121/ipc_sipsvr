/*
* mqinf.cpp ��
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h"  // NOLINT
#include "mqinf.h" // NOLINT
#include "pjproject.h" // NOLINT
#include "objs.h" // NOLINT

static const char* THIS_FILE = "mqinf.cpp";

MqInf* MqInf::_mqInf = NULL;
const char* _client_id = "sipsvc_mq_id";

int _reg_mid = 1;
int _pzt_mid = 2;
int _sdp_mid = 3;
int _sds_mid = 4;
int _live_stream = 5;
int _live_start = 6;
int _live_stop = 7;
int _plan_chg = 8;
int _plan_del = 9;
int _ipc_chg = 10;
int _sds_chg = 11;

 void mqConnectCallback(struct mosquitto *mosq, void *obj, int rc)
{
	int subQos = ConfigMgr::ins()->getMqSubQos();
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_reg_mid, IPC_REG_RESP, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_pzt_mid, PTZ_SEND_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_sdp_mid, SDP_SEND_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_sds_mid, SDS_INFO_RESP, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_live_stream, LIVE_MEDIA_STREAM, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_live_start, LIVE_START_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_live_stop, LIVE_STOP_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_plan_chg, PLAN_CHANGE_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_plan_del, PLAN_DEL_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_ipc_chg, IPC_CHG_REQ, subQos);
	mosquitto_subscribe(MqInf::ins()->_mos_t, &_sds_chg, SDS_CHG_REQ, subQos);
}

void mqDisconnectCallback(struct mosquitto *mosq, void *obj, int result)
{
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_reg_mid, IPC_REG_RESP);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_pzt_mid, PTZ_SEND_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_sdp_mid, SDP_SEND_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_sds_mid, SDS_INFO_RESP);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_live_stream, LIVE_MEDIA_STREAM);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_live_start, LIVE_START_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_live_stop, LIVE_STOP_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_plan_chg, PLAN_CHANGE_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_plan_del, PLAN_DEL_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_ipc_chg, IPC_CHG_REQ);
	mosquitto_unsubscribe(MqInf::ins()->_mos_t, &_sds_chg, SDS_CHG_REQ);
}

void mqMessageCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	if (MqInf::ins()) {
		MqInf::ins()->onMessageCallback(msg);
	}
}

static void mqSubscribeCallback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	SIP_LOG(SIP_TRACE, (THIS_FILE, "mq subscribe , mid:%d", mid));
}

MqInf::MqInf()
{
	_has_init = false;
	_mos_t = NULL;
	_mapLock = NULL;
	_mqInf = this;
}

MqInf::~MqInf()
{

}

int MqInf::init(pj_pool_t* pool)
{
	_pool = pool;
	if (!_has_init) {
		mosquitto_lib_init();
		const char* mqHost = ConfigMgr::ins()->getMqServerAddr();
		int mqPort = ConfigMgr::ins()->getMqServerPort();
		_mos_t = mosquitto_new(_client_id, true, NULL);
		mosquitto_connect_callback_set(_mos_t, mqConnectCallback);
		mosquitto_disconnect_callback_set(_mos_t, mqDisconnectCallback);
		mosquitto_message_callback_set(_mos_t, mqMessageCallback);
		mosquitto_subscribe_callback_set(_mos_t, mqSubscribeCallback);
		mosquitto_reconnect_delay_set(_mos_t, 3, 30, true);
		mosquitto_connect(_mos_t, mqHost, mqPort, 300);
		pj_lock_create_simple_mutex(_pool, "MqMap", &_mapLock);
		_has_init = true;
	}
	return 0;
}

void MqInf::unini()
{
	if (_has_init) {
		_has_init = false;
		mosquitto_loop_stop(_mos_t, true);
		mosquitto_disconnect(_mos_t);
		mosquitto_destroy(_mos_t);
		mosquitto_lib_cleanup();
		if (_mapLock) {
			pj_lock_destroy(_mapLock);
		}
	}
}

void MqInf::loopMqMsg()
{
	int msgCount = ConfigMgr::ins()->getMsgCount();
	int msgSize = ConfigMgr::ins()->getMsgSize();
	int rc = mosquitto_loop(_mos_t, 500, msgCount * msgSize);
	if (rc) {
		mosquitto_reconnect(_mos_t);
	}
}

/*
 * set MQ Message callback object
*/
void MqInf::setCallbackObject(const char* key, MqCallback*obj)
{
	if (key && obj) {
		pj_lock_acquire(_mapLock);
		_mapMsgCallback.insert(std::make_pair(std::string(key), obj));
		pj_lock_release(_mapLock);
	}
}

void MqInf::removeCallbackObject(const char* key, MqCallback* obj)
{
	if (key) {
		pj_lock_acquire(_mapLock);
		std::pair<MsgCallbackMap::iterator, MsgCallbackMap::iterator> p = _mapMsgCallback.equal_range(std::string(key));
		for (MsgCallbackMap::iterator it = p.first; it != p.second; ++it) {
			MqCallback* tmp = (MqCallback*)it->second;
			if (tmp && tmp == obj) {
				_mapMsgCallback.erase(it++);
				break;
			}
		}
		pj_lock_release(_mapLock);
	}
}

void MqInf::onMessageCallback(const struct mosquitto_message* msg)
{
	if (msg != NULL) {

		SIP_LOG(SIP_TRACE, (THIS_FILE, "mq message callback, mid:%d, payload:%s", msg->mid, (const char*)msg->payload));

		MqObject *obj = NULL;
		if (strcmp(msg->topic, IPC_REG_RESP) == 0) {
			obj = new ResponseObj();
			obj->fromJson((const char*)msg->payload);
			SIP_LOG(SIP_TRACE, (THIS_FILE, "receive register ipc response code:%d", ((ResponseObj*)obj)->getCode()));
		} else if (strcmp(msg->topic, PTZ_SEND_REQ) == 0) {
			obj = new PTZCmd();
		} else if (strcmp(msg->topic, SDP_SEND_REQ) == 0) {

		} else if (strcmp(msg->topic, SDS_INFO_RESP) == 0) {
			obj = new SdsResponse();
		} else if (strcmp(msg->topic, LIVE_MEDIA_STREAM) == 0) {
			obj = new LiveStreamResp();
		} else if (strcmp(msg->topic, LIVE_START_REQ) == 0) {
			obj = new LiveMediaStart();
		} else if (strcmp(msg->topic, LIVE_STOP_REQ) == 0) {
			obj = new LiveStopReq();
		} else if (strcmp(msg->topic, PLAN_CHANGE_REQ) == 0) {
			obj = new PlanChg();
		} else if (strcmp(msg->topic, PLAN_DEL_REQ) == 0) {
			obj = new PlanDel();
		} else if (strcmp(msg->topic, IPC_CHG_REQ) == 0) {
			obj = new IpcChgReq();
		} else if (strcmp(msg->topic, SDS_CHG_REQ) == 0) {
			obj = new SdsChgReq();
		}
		if (obj != NULL) {
			obj->fromJson(reinterpret_cast<const char*>(msg->payload));
			std::string topic = msg->topic;
			pj_lock_acquire(_mapLock);
			std::pair<MsgCallbackMap::iterator, MsgCallbackMap::iterator> p = _mapMsgCallback.equal_range(topic);
			for (MsgCallbackMap::iterator it = p.first; it != p.second; ++it) {
				it->second->onMsgCallback(topic.c_str(), obj);
			}
			pj_lock_release(_mapLock);
			delete obj;
		}
		
	}
}

int MqInf::publishMsg(const char* topic, MqObject* obj)
{
	int ret = 0;
	int pubQos = ConfigMgr::ins()->getMqPubQos();
	if (obj) {
		std::string jsonStr = obj->toJsonStr();
		SIP_LOG(SIP_TRACE, (THIS_FILE, "publish json string:%s", jsonStr.c_str()));
		for (int i = 0; i < 3; i++) {
			ret = mosquitto_publish(_mos_t, NULL, topic, jsonStr.length(), jsonStr.c_str(), pubQos, false);
			if (ret == MOSQ_ERR_SUCCESS) {
				break;
			} else {
				SIP_LOG(SIP_TRACE, (THIS_FILE, "publish fail!, try again, json string:%s", jsonStr.c_str()));
			}
		}
		if (ret != MOSQ_ERR_SUCCESS) {
			SIP_LOG(SIP_ERROR, (THIS_FILE, "publish fail!, try 3 times, json string:%s", jsonStr.c_str()));
		}
	}
	return 0;
}

