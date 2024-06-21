/*
* stream_mgr.cpp
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h" // NOLINT
#include "stream_mgr.h" // NOLINT

static const char* THIS_FILE = "stream_mgr.cpp";
StreamMgr* StreamMgr::_thr = NULL;

StreamMgr::StreamMgr()
{
	StreamMgr::_thr = this;
	_lock = NULL;
	_totalWriteRtpNumber = 0;
	_totalWriteUsec = 0;
	_lockWriteRtp = 0;
	_lockWriteUsec = 0;
}

StreamMgr::~StreamMgr()
{
}

void StreamMgr::init(pj_pool_t* pool)
{
	_pool = pool;
	pj_lock_create_simple_mutex(_pool, NULL, &_lock);
	pj_lock_create_simple_mutex(_pool, NULL, &_lockWriteRtp);
	pj_lock_create_simple_mutex(_pool, NULL, &_lockWriteUsec);
	MqInf::ins()->setCallbackObject(SDS_INFO_RESP, this);
	MqInf::ins()->setCallbackObject(PLAN_CHANGE_REQ, this);
	MqInf::ins()->setCallbackObject(PLAN_DEL_REQ, this);
	MqInf::ins()->setCallbackObject(IPC_CHG_REQ, this);
	MqInf::ins()->setCallbackObject(SDS_CHG_REQ, this);
}

void StreamMgr::uninit()
{
	TRACE_FUNC("StreamMgr::uninit()");

	MqInf::ins()->removeCallbackObject(SDS_INFO_RESP, this);
	MqInf::ins()->removeCallbackObject(PLAN_CHANGE_REQ, this);
	MqInf::ins()->removeCallbackObject(PLAN_DEL_REQ, this);
	MqInf::ins()->removeCallbackObject(IPC_CHG_REQ, this);
	MqInf::ins()->removeCallbackObject(SDS_CHG_REQ, this);

	pj_lock_acquire(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.begin();
	for (; it != _mapStreamChannel.end(); ++it) {
		if (it->second->_streamFile) {
			delete it->second->_streamFile;
		}
		delete it->second;
	}
	_mapStreamChannel.clear();
	pj_lock_release(_lock);

	if (_lock) {
		pj_lock_destroy(_lock);
		_lock = NULL;
	}
	if(_lockWriteRtp) {
		pj_lock_destroy(_lockWriteRtp);
		_lockWriteRtp = NULL;
	}
	if(_lockWriteUsec) {
		pj_lock_destroy(_lockWriteUsec);
		_lockWriteUsec = NULL;
	}
}

void StreamMgr::putWriteFileUsec(pj_uint32_t usec)
{
	LockGuard guard(_lockWriteUsec);
	_totalWriteUsec += usec;
}

void StreamMgr::putWriteFileLen(pj_uint32_t len)
{
	LockGuard guard(_lockWriteRtp);
	_totalWriteRtpNumber += len;
}

/*
 * add a new Ipc values in map
*/
void StreamMgr::addIpcChannel(const char* ipcAddr, int ipcPort, const char* fmt)
{
	TRACE_FUNC("StreamMgr::addIpcChannel()1");

	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it == _mapStreamChannel.end()) {
		StreamChannel* obj = new StreamChannel();
		obj->_fmt = (fmt == NULL ? "" : fmt);
		obj->_streamFile = new StreamFile();
		obj->_fileSize = 0.0;
		obj->_volume = 0.0;
		obj->_needLive = false;
		obj->_flag = 0;
		obj->_liveCli = NULL;
		_mapStreamChannel.insert(std::make_pair(ipcUrl, obj));
		this->publishSdsInfo(ipcUrl);
	}
	else {
		it->second->_fmt = (fmt == NULL ? "" : fmt);
	}
}

void StreamMgr::addIpcChannel(const char* ipcAddr, int ipcPort, bool needLive, LiveRtpCli* live)
{
	TRACE_FUNC("StreamMgr::addIpcChannel()2");

	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	addIpcChannelByIpcUrl(ipcUrl.c_str(), needLive, live);
}

void StreamMgr::addIpcChannelByIpcUrl(const char* ipcUrl, bool needLive, LiveRtpCli* live)
{
	TRACE_FUNC("StreamMgr::addIpcChannelByIpcUrl()");

	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		it->second->_needLive = needLive;
		it->second->_liveCli = live;
	} else {
		StreamChannel* obj = new StreamChannel();
		obj->_streamFile = new StreamFile();
		obj->_fileSize = 0.0;
		obj->_volume = 0.0;
		obj->_flag = 0;
		obj->_needLive = needLive;
		obj->_liveCli = live;
		_mapStreamChannel.insert(std::make_pair(ipcUrl, obj));
		this->publishSdsInfo(ipcUrl);
	}
}

void StreamMgr::setFmt(const char* ipcAddr, int ipcPort, const char* fmt)
{
	TRACE_FUNC("StreamMgr::setFmt()");
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		it->second->_fmt = fmt;
	}
}

void StreamMgr::setLiveValue(const char* ipcAddr,int ipcPort, bool needLive, LiveRtpCli* cli)
{
	TRACE_FUNC("StreamMgr::setLiveValue()");
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		it->second->_needLive = needLive;
		it->second->_liveCli = cli;
	}
}

LiveRtpCli* StreamMgr::getLiveCli(const char* ipcAddr, int ipcPort)
{
	TRACE_FUNC("StreamMgr::getLiveCli()");
	LiveRtpCli* cli = NULL;
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		cli = it->second->_liveCli;
	}
	return cli;
}

LiveRtpCli* StreamMgr::getLiveCliByIpcUrl(const char* ipcUrl)
{
	TRACE_FUNC("StreamMgr::getLiveCliByIpcUrl()");
	LiveRtpCli* cli = NULL;
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		cli = it->second->_liveCli;
	}
	return cli;
}

const char* StreamMgr::getFmt(const char* ipcAddr, int ipcPort)
{
	TRACE_FUNC("StreamMgr::getFmt()");
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	return getFmtByIpcUrl(ipcUrl.c_str());
}

const char* StreamMgr::getFmtByIpcUrl(const char* ipcUrl)
{
	TRACE_FUNC("StreamMgr::getFmtByIpcUrl()");
	const char* fmt = NULL;
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		fmt = it->second->_fmt.c_str();
	}
	return fmt;
}
/*
 * find streamchannel object by ipcAddr
*/
StreamChannel* StreamMgr::getChannelByIpc(const char* ipcAddr, int ipcPort)
{
	TRACE_FUNC("StreamMgr::getChannelByIpc()");
	StreamChannel* obj = NULL;
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		obj = it->second;
	}
	return obj;
}
/*
 * Accept RTP payload from outside
 * and find the channel by key ipcAddr
 * call streamFile to save RTP payload into file or translate to live client
*/
int StreamMgr::translateRtpPayload(const char* ipcAddr, int ipcPort, const char* payload, int paylen)
{
	int ret = -1;
	StreamFile* streamFile = NULL;
	LiveRtpCli* cli = NULL;
	std::string path;
	std::string fmt;
	double volume = 0.0;
	double fileSize = 0.0;
	int flag = 0;
	bool needLive = false;
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	{
		LockGuard guard(_lock);
		StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
		if (it != _mapStreamChannel.end()) {
			streamFile = it->second->_streamFile;
			cli = it->second->_liveCli;
			needLive = it->second->_needLive;
			path = it->second->_filePath;
			fmt = it->second->_fmt;
			volume = it->second->_volume;
			fileSize = it->second->_fileSize;
			flag = it->second->_flag;
			if (streamFile) {
				ret = saveStreamToFile(ipcUrl, streamFile, it->second->_planId, flag, path, fmt, volume, fileSize, payload, paylen);
			}
			if (needLive && cli) {
				cli->sendRtp(ipcUrl.c_str(), payload, paylen);
			}
		}
	}
	return ret;
}

void StreamMgr::removeLiveCli(const char* ipcAddr, int ipcPort)
{
	TRACE_FUNC("StreamMgr::removeLiveCli()");
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	removeLiveCliByIpcUrl(ipcUrl.c_str());
}

void StreamMgr::removeLiveCliByIpcUrl(const char* ipcUrl)
{
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		if (it->second->_liveCli) {
			it->second->_liveCli->close();
			delete it->second->_liveCli;
			it->second->_liveCli = NULL;
			it->second->_needLive = false;
		}
	}
}

void StreamMgr::removeIpcChannel(const char* ipcAddr, int ipcPort)
{
	TRACE_FUNC("StreamMgr::removeIpcChannel()"); 
	std::string ipcUrl = createIpcUrl(ipcAddr, ipcPort);
	LockGuard guard(_lock);
	StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
	if (it != _mapStreamChannel.end()) {
		if (it->second->_streamFile) {
			delete it->second->_streamFile;
		}
		if (it->second->_liveCli) {
			it->second->_liveCli->close();
		}
		delete it->second;
		_mapStreamChannel.erase(it);
	}
}
/*
 * save stream into file
*/
int StreamMgr::saveStreamToFile(const std::string& ipcUrl,
	StreamFile* file,
	const std::string& planId,
	int flag,
	const std::string& path,
	const std::string& fmt,
	double volume,
	double fileSize,
	const char* payload,
	int len)
{
	if (len <= 0) {
		return -1;
	}
	if (path.length() == 0) {
		//publishSdsInfo(ipcUrl);
		return 0;
	}
	if (flag == 0) {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "The flag is 0 so don't save data into file."));
		return -1;
	}
	file->writeStream((const char*)payload, len, ipcUrl.c_str(), path.c_str(), fmt.c_str(), fileSize, planId.c_str());
	return 0;
}

/*
 * send a MQ publish for getting SDS detail
*/
void StreamMgr::publishSdsInfo(std::string ipcUrl)
{
	if (ipcUrl.length() > 0) {
		SdsInfoReq obj;
		obj.setIpcAddr(ipcUrl.c_str());
		MqInf::ins()->publishMsg(SDS_INFO_REQ, &obj);
	}
}

/*
 * Callback function to receive subscribe message from MQ
*/
void StreamMgr::onMsgCallback(const char* topic, MqObject* obj)
{
	if (obj && topic) {
		if (strcmp(topic, SDS_INFO_RESP) == 0) {
			SdsResponse* sdsResp = (SdsResponse*)obj;
			std::string ipcUrl = sdsResp->getIpcAddr();
			std::string filePath = sdsResp->getFilePath();
			double volume = sdsResp->getVolume();
			double fileSize = sdsResp->getFileSize();
			int flag = sdsResp->getFlag();
			LockGuard guard(_lock);
			StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
			if (it != _mapStreamChannel.end()) {
				it->second->_filePath = filePath;
				it->second->_volume = volume;
				it->second->_fileSize = fileSize;
				it->second->_flag = flag;
				it->second->_planId = sdsResp->getPlanId();
			}
		} else if (strcmp(topic, PLAN_CHANGE_REQ) == 0) {
			PlanChg* planObj = (PlanChg*)obj;
			std::string ipcUrl = planObj->getIpcAddr();
			LockGuard guard(_lock);
			StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
			if (it != _mapStreamChannel.end()) {
				it->second->_volume = planObj->getVolume();
				it->second->_fileSize = planObj->getFileSize();
				it->second->_flag = planObj->getFlag();
				it->second->_filePath = planObj->getPath();
				it->second->_planId = planObj->getPlanId();
			}
		} else if (strcmp(topic, PLAN_DEL_REQ) == 0) {
			PlanDel* planObj = (PlanDel*)obj;
			std::string ipcUrl = planObj->getIpcAddr();
			LockGuard guard(_lock);
			StreamChannelMap::iterator it = _mapStreamChannel.find(ipcUrl);
			if (it != _mapStreamChannel.end()) {
				it->second->_flag = 0;
			}
		} else if (strcmp(topic, IPC_CHG_REQ) == 0) {
			IpcChgReq * ipcObj = (IpcChgReq*)obj;
			std::string oldAddr = ipcObj->getIpcOldAddr();
			std::string newAddr = ipcObj->getIpcNewAddr();
			LockGuard guard(_lock);
			StreamChannelMap::iterator it = _mapStreamChannel.find(oldAddr);
			if (it != _mapStreamChannel.end()) {
				StreamChannel* channel = it->second;
				StreamChannelMap::iterator itNew = _mapStreamChannel.find(newAddr);
				if (itNew != _mapStreamChannel.end()) {
					delete itNew->second;
					_mapStreamChannel.erase(itNew);
				}
				_mapStreamChannel.insert(std::make_pair(newAddr, channel));
				_mapStreamChannel.erase(it);
			}
		} else if (strcmp(topic, SDS_CHG_REQ) == 0) {
			SdsChgReq * sdsObj = (SdsChgReq*)obj;
			std::string oldPath = sdsObj->getOldPath();
			std::string newPath = sdsObj->getNewPath();
			std::string ipcAddr = sdsObj->getIpcAddr();
			LockGuard guard(_lock);
			StreamChannelMap::iterator it = _mapStreamChannel.find(ipcAddr);
			if (it != _mapStreamChannel.end()) {
				it->second->_filePath = newPath;
			}
		}
	}
}
