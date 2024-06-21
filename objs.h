#pragma once

#include <string>
#include <stdint.h>

class MqObject {
public:
	virtual int fromJson(const char* jsonStr) = 0;
	virtual std::string toJsonStr() = 0;
};

class ResponseObj : public MqObject
{
private:
	int _code;
	uint32_t _sn;
	std::string _msg;
public:
	int getCode() { return _code; }
	void setCode(int code) { this->_code = code; }
	void setSn(pj_int64_t sn) { this->_sn = sn;  }
	int getSn() { return this->_sn; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class RegRequest : public MqObject
{
private:
	uint32_t _sn;
	std::string _sipId;
	std::string _userName;
	std::string _passwd;
	std::string _ipcAddr;
	std::string _url;
	std::string _realm;
	std::string _response;
public:
	void setSn(pj_int64_t sn) { _sn = sn; }
	void setSipId(const char* val) { if (val != NULL) { _sipId = std::string(val); } }
	void setUserName(const char* val) { if (val != NULL) { _userName = std::string(val); } }
	void setPassword(const char* val) { if (val != NULL) { _passwd = std::string(val); } }
	void setIpcAddr(const char* val) { if (val != NULL) { _ipcAddr = std::string(val); } }
	void setRealm(const char* val) { if (val != NULL) _realm = val; }
	void setResponse(const char* val) { if (val != NULL) _response = val; }
	void setUrl(const char* val) { if (val != NULL) _url = val; }
	const char* getUserName() { return _userName.c_str(); }
	const char* getSipId() { return _sipId.c_str(); }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	const char* getRealm() { return _realm.c_str(); }
	const char* getResponse() { return _response.c_str(); }
	const char* getUrl() { return _url.c_str(); }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class SdsInfoReq : public MqObject
{
private:
	std::string _ipcAddr;
public:
	void setIpcAddr(const char* val) { if (val) _ipcAddr = val; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class SdsResponse : public MqObject
{
private:
	std::string _planId;
	std::string _ipcAddr;
	std::string _filePath;
	double _volume;
	double _fileSize;
	double _used;
	int _flag;
public:
	void setPlanId(const char* id) { _planId = id; }
	const char* getPlanId() { return _planId.c_str(); }
	void setIpcAddr(const char* val) { if (val) _ipcAddr = val; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	const char* getFilePath() { return _filePath.c_str(); }
	double getVolume() { return _volume; }
	double getFileSize() { return _fileSize; }
	double getUsed() { return _used; }
	void setFlag(int flag) { this->_flag = flag; }
	int getFlag() { return _flag; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class FileNameRpt : public MqObject
{
private:
	std::string _planId;
	std::string _ipcAddr;
	std::string _filePath;
	std::string _fileName;
	double _fileSize;

public:
	void setPlanId(const char* id) { _planId = id; }
	const char* getPlanId() { return _planId.c_str(); }
	void setIpcAddr(const char* val) { if (val) _ipcAddr = val; }
	const char* getIpcAddr() { return _ipcAddr.c_str();  }
	void setFilePath(const char* val) { if (val) _filePath = val; }
	const char* getFilePath() { return _filePath.c_str(); }
	void setFileName(const char* name) { if (name) _fileName = name; }
	const char* getFileName() { return _fileName.c_str();}
	void setFileSize(double size) { _fileSize = size; }
	double getFileSize() { return _fileSize; }

	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class LiveStreamResp : public MqObject
{
private:
	std::string _ipcAddr;
	std::string _url;

public:
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	void setUrl(const char* val) { if (val) _url = val; }
	const char* getUrl() { return _url.c_str(); }

	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class LiveMediaStart : public MqObject
{
private:
	std::string _svcAddr;
	std::string _ipcAddr;
	int _startPort;
	int _endPort;
public:
	void setSvcAddr(const char* addr) { if (addr) _svcAddr = addr; }
	const char* getSvcAddr() { return _svcAddr.c_str(); }
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	void setStartPort(int port) {
		_startPort = port;
	}
	int getStartPort() { return _startPort;  }
	void setEndPort(int port) {
		_endPort = port;
	}
	int getEndPort() { return _endPort;  }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class LiveMediaStartResp : public MqObject
{
private:
	std::string _cliAddr;
	std::string _ipcAddr;
	std::string _fmt;
	int _startPort;
	int _endPort;
public:
	void setCliAddr(const char* addr) { if (addr) _cliAddr = addr; }
	const char* getCliAddr() { return _cliAddr.c_str(); }
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	void setFmt(const char* fmt) { if (fmt) _fmt = fmt; }
	const char* getFmt() { return _fmt.c_str(); }
	void setStartPort(int port) {
		_startPort = port;
	}
	int getStartPort() { return _startPort; }
	void setEndPort(int port) {
		_endPort = port;
	}
	int getEndPort() { return _endPort; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class LiveStopReq : public MqObject
{
private:
	std::string _svcAddr;
	std::string _ipcAddr;
	int _startPort;
	int _endPort;
public:
	void setSvcAddr(const char* addr) { if (addr) _svcAddr = addr; }
	const char* getSvcAddr() { return _svcAddr.c_str(); }
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	void setStartPort(int port) {
		_startPort = port;
	}
	void setEndPort(int port) {
		_endPort = port;
	}
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class PTZCmd : public MqObject
{
private:
	std::string _ipcAddr;
	int _cmd;
public:
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	void setCmd(int cmd) { _cmd = cmd; }
	int getCmd() { return _cmd; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class PlanChg : public MqObject
{
private:
	std::string _planId;
	std::string _path;
	int _flag;
	std::string _ipcAddr;
	double _volume;
	double _fileSize;
public:
	void setPlanId(const char* id) { this->_planId = id; }
	const char* getPlanId() { return _planId.c_str(); }
	void setPath(const char* path) { if (path) _path = path; }
	const char* getPath() { return _path.c_str(); }
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	void setFlag(int flag) { this->_flag = flag; }
	int getFlag() { return _flag; }
	double getVolume() { return _volume; }
	double getFileSize() { return _fileSize; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class PlanDel : public MqObject
{
private:
	std::string _planId;
	int _flag;
	std::string _ipcAddr;
public:
	void setPlanId(const char* id) { this->_planId = id; }
	const char* getPlanId() { return _planId.c_str(); }
	void setFlag(int flag) { this->_flag = flag; }
	int getFlag() { return _flag; }
	void setIpcAddr(const char* addr) { if (addr) _ipcAddr = addr; }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class StatusRpt : public MqObject
{
private:
	std::string _ipcAddr;
	int _status;
public:
	void setIpcAddr(const char* addr) { this->_ipcAddr = addr; }
	void setStatus(int status) { this->_status = status; }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class IpcChgReq : public MqObject
{
private:
	std::string _ipcOldAddr;
	std::string _ipcNewAddr;
public:
	const char* getIpcOldAddr() { return _ipcOldAddr.c_str();  }
	const char* getIpcNewAddr() { return _ipcNewAddr.c_str(); }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};

class SdsChgReq : public MqObject
{
private:
	std::string _oldPath;
	std::string _newPath;
	std::string _ipcAddr;
public:
	const char* getOldPath() { return _oldPath.c_str(); }
	const char* getNewPath() { return _newPath.c_str(); }
	const char* getIpcAddr() { return _ipcAddr.c_str(); }
	int fromJson(const char* jsonStr);
	std::string toJsonStr();
};
