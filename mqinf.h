#pragma once

#include "mosquitto.h"
#include <map>


#define IPC_REG_REQ     "IPCRegisterReq"
#define IPC_REG_RESP    "IPCRegisterResp"
#define PTZ_SEND_REQ    "PZTSendReq"
#define SDP_SEND_REQ    "SDPSendReq"
#define SDS_INFO_REQ    "SDSInfoReq"
#define SDS_INFO_RESP   "SDSInfoResp"
#define FILE_NAME_REQ   "FileNameReport"

#define LIVE_MEDIA_STREAM "LiveMediaStream"
#define LIVE_START_REQ  "LiveMediaStartReq"
#define LIVE_START_RESP "LiveMediaStartResp"
#define LIVE_STOP_REQ   "LiveMediaStopReq"
#define PLAN_CHANGE_REQ "PlanChgReq"
#define PLAN_DEL_REQ    "PlanDelReq"
#define IPC_STATUS_RPT "IpcStatusReport"
#define IPC_CHG_REQ "IpcChangeReq"
#define SDS_CHG_REQ "SdsChangeReq"

class MqCallback
{
public:
	virtual void onMsgCallback(const char* topic, MqObject* obj) = 0;
};

/***************************************************************
 *  MqInf class
 *  support mqtt interface 
 *  
****************************************************************/
class MqInf
{
	typedef std::multimap<std::string, MqCallback*> MsgCallbackMap;
public:
	MqInf();
	~MqInf();

	int init(pj_pool_t* pool);
	void unini();
	int publishMsg(const char* topic, MqObject* obj);
	void setCallbackObject(const char* key, MqCallback*obj);
	void removeCallbackObject(const char* key, MqCallback* obj);
	void loopMqMsg();
	static MqInf* ins() { return _mqInf; }
private:
	friend int mqWorkerThreadProc(void *arg);
	friend void mqConnectCallback(struct mosquitto *mosq, void *obj, int rc);
	friend void mqDisconnectCallback(struct mosquitto *mosq, void *obj, int result);
	friend void mqMessageCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
	void onMessageCallback(const struct mosquitto_message* msg);
private:
	bool _has_init;
	struct mosquitto * _mos_t;
	pj_lock_t  * _mapLock;
	MsgCallbackMap _mapMsgCallback;
	static MqInf* _mqInf;
	pj_pool_t *_pool;
};

