#pragma once
#include "pjproject.h"
#include "mqinf.h"
#include "stream_file.h"
#include "live_rtp_cli.h"

/*
 * singleton class 
 * create stream channel based on every IPC address
 * accept IPC stream from IpcCall class and save to file or live broadcast.
*/
typedef struct StreamChannel
{
	StreamFile* _streamFile;
	bool _needLive;
	std::string _planId;
	std::string _fmt;
	LiveRtpCli* _liveCli;
	std::string _filePath;
	double      _volume;
	double      _fileSize;
	int         _flag;   //if or not activated
}StreamChannel;

class StreamMgr: public MqCallback
{

	typedef std::map<std::string, StreamChannel*> StreamChannelMap;
public:
	StreamMgr();
	~StreamMgr();
	void init(pj_pool_t* pool);
	void uninit();
	void addIpcChannel(const char* ipcAddr, int ipcPort, const char* fmt = NULL);
	void addIpcChannel(const char* ipcAddr, int ipcPort, bool needLive, LiveRtpCli* live);
	void addIpcChannelByIpcUrl(const char* ipcUrl,bool needLive, LiveRtpCli* live);
	void setLiveValue(const char* ipcAddr, int ipcPort, bool needLive, LiveRtpCli* cli);
	LiveRtpCli* getLiveCli(const char* ipcAddr, int ipcPort);
	LiveRtpCli* getLiveCliByIpcUrl(const char* ipcUrl);
	StreamChannel* getChannelByIpc(const char* ipcAddr, int ipcPort);
	void setFmt(const char* ipcAddr, int ipcPort, const char* fmt);
	const char* getFmt(const char* ipcAddr, int ipcPort);
	const char* getFmtByIpcUrl(const char* ipcUrl);
	int translateRtpPayload(const char* ipcAddr, int ipcPort, const char* payload, int paylen);
	void removeIpcChannel(const char* ipcAddr, int ipcPort);
	void removeLiveCli(const char* ipcAddr, int ipcPort);
	void removeLiveCliByIpcUrl(const char* ipcUrl);
	void putWriteFileUsec(pj_uint32_t usec);
	pj_uint32_t getWriteFileUsec() { return _totalWriteUsec; }
	void putWriteFileLen(pj_uint32_t len);
	pj_uint32_t getWriteFileLen() { return _totalWriteRtpNumber; }
	static StreamMgr* ins() { return _thr;  }
private:
	std::string createIpcUrl(const char* ipcAddr, int ipcPort) {
		char val[64];
		memset(val, 0, 64);
		pj_ansi_snprintf(val, sizeof(val) - 1, "%s:%d", ipcAddr, ipcPort);
		return std::string(val);
	}
	void publishSdsInfo(std::string ipcUrl);
	void onMsgCallback(const char* topic, MqObject* obj);
	int saveStreamToFile(const std::string& ipcUrl, StreamFile* file, const std::string& planId, int flag, const std::string& path, const std::string& fmt, double volume, double fileSize, const char* payload, int len);
private:
	static StreamMgr * _thr;
private:
	pj_pool_t * _pool;
	pj_lock_t *_lock;
	pj_lock_t * _lockWriteRtp;
	pj_lock_t* _lockWriteUsec;
	StreamChannelMap _mapStreamChannel;
	pj_uint32_t _totalWriteRtpNumber;
	pj_uint32_t _totalWriteUsec;
};
