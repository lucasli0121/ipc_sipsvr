#pragma once

#include <vector>

/* Codec descriptor: */
struct codec
{
	int    pt;
	char   name[32];
	int    clock_rate;
	int    bit_rate;
	int    frame;
	char   desc[32];
};

class ConfigMgr
{
public:
	ConfigMgr();
	~ConfigMgr();
	void setFileBufSize(int size) { _fileBufSize = size; }
	int getFileBufSize() { return _fileBufSize; }
	void setWriteFileFreq(int freq) { _writeFileFreq = freq; }
	int getWriteFileFreq() { return _writeFileFreq; }
	void setLogFileSize(int size) { _logFileSize = size; }
	int getLogFileSize() { return _logFileSize; }
	void setSipHost(const char* host) { _sipHost = host;  }
	const char* getSipHost() { return _sipHost.c_str(); }
	void setSipPort(int port) { _sipPort = port; }
	int getSipPort() { return _sipPort; }
	void setSipId(const char* id) { _sipId = id; }
	const char* getSipId() { return _sipId.c_str(); }
	void setStartRtpPort(int startPt) { _startRtpPort = startPt; }
	int getStartRtpPort() { return _startRtpPort; }
	void setRtpPortStep(int step) { _rtpPortStep = step; }
	int getRtpPortStep() { return _rtpPortStep; }
	void setVideoFmt(const char* fmt) { _videoFormat = fmt; }
	const char* getVideoFmt() { return _videoFormat.c_str(); }
	int getVideoCodec(std::vector<codec> &vec);
	void setUseTcp(bool use) { _useTcp = use; }
	bool getUseTcp() { return _useTcp; }
	void setUseUdp(bool use) { _useUdp = use; }
	bool getUseUdp() { return _useUdp; }
	void setFrame(int frame) { _frame = frame; }
	int getFrame() { return _frame; }
	void setBitRate(int rate) { _bitRate = rate; }
	int getBitRate() { return _bitRate; }
	void setClockRate(int rate) { _clockRate = rate; }
	int getClockRate() { return _clockRate; }
	void setMqServerAddr(const char* addr, int port);
	void setMqParams(int subQos, int pubQos, int msgCount, int msgSize);
	const char* getMqServerAddr() { return _mqServerAddr.c_str(); }
	int getMqServerPort() { return _mqPort; }
	int getMqSubQos() { return _subQos; }
	int getMqPubQos() { return _pubQos; }
	int getMsgCount() { return _msgCount; }
	int getMsgSize() { return _msgSize; }
	void setRegAuth(bool auth) { _regAuth = auth; }
	bool getRegAuth() { return _regAuth; }
	void setSipUseTcp(bool use) { _sipUseTcp = use; }
	bool getSipUseTcp() { return _sipUseTcp;}
	void setForceKeyFrame(bool force) { _forceKeyFrame = force; }
	bool getForceKeyFrame() { return _forceKeyFrame; }
	void setChannelId(const char* id) { _channelId = id; }
	const char* getChannelId() { return _channelId.c_str(); }
	int getTimeout() { return _timeout; }
	void setTimeout(int tmOut) { _timeout = tmOut; }
	const char* getRtpRecvIp() { return _rtpRecvIp.c_str(); }
	void setRtpRecvIp(const char* ip) { _rtpRecvIp = ip; }
	static ConfigMgr* ins() { return _mgr; }
private:
	int _fileBufSize;
	int _writeFileFreq;
	int _logFileSize;
	std::string _sipHost;
	int _sipPort;
	std::string _sipId;
	std::string _channelId;
	std::string _rtpRecvIp;
	int _startRtpPort;
	int _rtpPortStep;
	std::string _videoFormat;
	std::string _mqServerAddr;
	int _frame;
	int _bitRate;
	int _clockRate;
	int _mqPort;
	int _pubQos;
	int _subQos;
	int _msgCount;
	int _msgSize;
	bool _useTcp;
	bool _useUdp;
	bool _sipUseTcp;
	bool _regAuth;
	bool _forceKeyFrame;
	int _timeout;
	static ConfigMgr* _mgr;
};
