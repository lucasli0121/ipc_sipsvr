#include "pch.h"
#include "objs.h"
#include "json/json.h"

int ResponseObj::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	
	if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
		_code = root["code"].asInt();
		_msg = root["message"].asString();
		_sn = root["sn"].asUInt();
		return 0;
	}
	return -1;
}

std::string ResponseObj::toJsonStr()
{
	Json::Value root;
	root["code"] = _code;
	root["message"] = _msg;
	root["sn"] = _sn;
	
	return root.toStyledString();
}

int RegRequest::fromJson(const char* jsonStr)
{
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	int len = strlen(jsonStr);
	Json::String err;
	if (reader->parse(jsonStr, jsonStr + len, &root, &err)) {
		_sn = root["sn"].asUInt();
		_sipId = root["sipId"].asString();
		_userName = root["username"].asString();
		_passwd = root["password"].asString();
		_ipcAddr = root["address"].asString();
		_realm = root["realm"].asString();
		_response = root["response"].asString();
		_url = root["url"].asString();
	}
	return 0;
}

std::string RegRequest::toJsonStr()
{
	Json::Value root;
	root["sn"] = _sn;
	root["sipId"] = _sipId;
	root["username"] = _userName;
	root["password"] = _passwd;
	root["address"] = _ipcAddr;
	root["realm"] = _realm;
	root["response"] = _response;
	root["url"] = _url;
	return root.toStyledString();
}

int SdsInfoReq::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
		_ipcAddr = root["ipcAddr"].asString();
	}
	return 0;
}

std::string SdsInfoReq::toJsonStr()
{
	Json::Value root;
	root["ipcAddr"] = _ipcAddr;
	return root.toStyledString();
}

int SdsResponse::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			_planId = root["planId"].asString();
			_ipcAddr = root["ipcAddr"].asString();
			_filePath = root["filePath"].asString();
			std::string volume = root["volume"].asString();
			_volume = strtod(volume.c_str(), NULL);
			std::string fileSize = root["fileSize"].asString();
			_fileSize = strtod(fileSize.c_str(), NULL);
			std::string used = root["used"].asString();
			_used = strtod(used.c_str(), NULL);
			_flag = root["flag"].asInt();
		}
	}
	catch (...) {

	}
	return 0;
}
std::string SdsResponse::toJsonStr()
{
	Json::Value root;
	root["planId"] = _planId;
	root["ipcAddr"] = _ipcAddr;
	root["filePath"] = _filePath;
	root["volume"] = _volume;
	root["fileSize"] = _fileSize;
	root["flag"] = _flag;
	root["used"] = _used;
	return root.toStyledString();
}

int FileNameRpt::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			_planId = root["planId"].asString();
			_ipcAddr = root["ipcAddr"].asString();
			_filePath = root["filePath"].asString();
			_fileName = root["fileName"].asString();
			std::string fileSize = root["fileSize"].asString();
			_fileSize = strtod(fileSize.c_str(), NULL);
		}
	}
	catch (...) {

	}
	return 0;
}
std::string FileNameRpt::toJsonStr()
{
	Json::Value root;
	root["planId"] = _planId;
	root["ipcAddr"] = _ipcAddr;
	root["filePath"] = _filePath;
	root["fileName"] = _fileName;
	root["fileSize"] = _fileSize;
	return root.toStyledString();
}


int LiveStreamResp::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			_ipcAddr = root["ipcAddr"].asString();
			_url = root["url"].asString();
		}
	}
	catch (...) {

	}
	return 0;
}
std::string LiveStreamResp::toJsonStr()
{
	Json::Value root;
	root["ipcAddr"] = _ipcAddr;
	root["url"] = _url;
	return root.toStyledString();
}

int LiveMediaStart::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			_svcAddr = root["svcAddr"].asString();
			_ipcAddr = root["ipcAddr"].asString();
			_startPort = root["startPort"].asInt();
			_endPort = root["endPort"].asInt();
		}
	}
	catch (...) {

	}
	return 0;
}

std::string LiveMediaStart::toJsonStr()
{
	Json::Value root;
	root["svcAddr"] = _svcAddr;
	root["ipcAddr"] = _ipcAddr;
	root["startPort"] = _startPort;
	root["endPort"] = _endPort;
	return root.toStyledString();
}

int LiveMediaStartResp::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			_cliAddr = root["svcAddr"].asString();
			_ipcAddr = root["ipcAddr"].asString();
			_fmt = root["fmt"].asString();
			_startPort = root["startPort"].asInt();
			_endPort = root["endPort"].asInt();
		}
	}
	catch (...) {

	}
	return 0;
}

std::string LiveMediaStartResp::toJsonStr()
{
	Json::Value root;
	root["cliAddr"] = _cliAddr;
	root["ipcAddr"] = _ipcAddr;
	root["fmt"] = _fmt;
	root["startPort"] = _startPort;
	root["endPort"] = _endPort;
	return root.toStyledString();
}


int LiveStopReq::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			_svcAddr = root["svcAddr"].asString();
			_ipcAddr = root["ipcAddr"].asString();
			_startPort = root["startPort"].asInt();
			_endPort = root["endPort"].asInt();
		}
	}
	catch (...) {

	}
	return 0;
}

std::string LiveStopReq::toJsonStr()
{
	Json::Value root;
	root["svcAddr"] = _svcAddr;
	root["ipcAddr"] = _ipcAddr;
	root["startPort"] = _startPort;
	root["endPort"] = _endPort;
	return root.toStyledString();
}

int PTZCmd::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			if (root.size() == 0) {
				return -1;
			}
			_ipcAddr = root["ipcAddr"].asString();
			_cmd = root["direct"].asInt();
		}
	} catch (...) {

	}
	return 0;
}

std::string PTZCmd::toJsonStr()
{
	Json::Value root;
	root["ipcAddr"] = _ipcAddr;
	root["direct"] = _cmd;
	return root.toStyledString();
}

int PlanChg::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			if (root.size() == 0) {
				return -1;
			}
			_planId = root["planId"].asString();
			_flag = root["flag"].asInt();
			std::string volume = root["volume"].asString();
			_volume = strtod(volume.c_str(), NULL);
			std::string fileSize = root["fileSize"].asString();
			_fileSize = strtod(fileSize.c_str(), NULL);
			_ipcAddr = root["ipcAddr"].asString();
			_path = root["path"].asString();
		}
	} catch (...) {

	}
	return 0;
}
std::string PlanChg::toJsonStr()
{
	Json::Value root;
	root["planId"] = _planId;
	root["flag"] = _flag;
	root["volume"] = _volume;
	root["fileSize"] = _fileSize;
	root["ipcAddr"] = _ipcAddr;
	root["path"] = _path;
	return root.toStyledString();
}

int PlanDel::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			if (root.size() == 0) {
				return -1;
			}
			_planId = root["planId"].asString();
			_flag = root["flag"].asInt();
			_ipcAddr = root["ipcAddr"].asString();
		}
	} catch (...) {

	}
	return 0;
}

std::string PlanDel::toJsonStr()
{
	Json::Value root;
	root["planId"] = _planId;
	root["flag"] = _flag;
	root["ipcAddr"] = _ipcAddr;
	return root.toStyledString();
}

int StatusRpt::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			if (root.size() == 0) {
				return -1;
			}
			_ipcAddr = root["address"].asString();
			_status = root["status"].asInt();
		}
	} catch (...) {

	}
	return 0;
}
std::string StatusRpt::toJsonStr()
{
	Json::Value root;
	root["address"] = _ipcAddr;
	root["status"] = _status;
	return root.toStyledString();
}

int IpcChgReq::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			if (root.size() == 0) {
				return -1;
			}
			_ipcOldAddr = root["oldAddr"].asString();
			_ipcNewAddr = root["newAddr"].asString();
		}
	} catch (...) {

	}
	return 0;
}
std::string IpcChgReq::toJsonStr()
{
	Json::Value root;
	root["oldAddr"] = _ipcOldAddr;
	root["newAddr"] = _ipcNewAddr;
	return root.toStyledString();
}

int SdsChgReq::fromJson(const char* jsonStr)
{
	Json::String err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value root;
	if (jsonStr == NULL) {
		return -1;
	}
	try {
		if (reader->parse(jsonStr, jsonStr + strlen(jsonStr), &root, &err)) {
			if (root.size() == 0) {
				return -1;
			}
			_oldPath = root["oldPath"].asString();
			_newPath = root["newPath"].asString();
			_ipcAddr = root["ipcAddr"].asString();
		}
	} catch (...) {

	}
	return 0;
}
std::string SdsChgReq::toJsonStr()
{
	Json::Value root;
	root["oldPath"] = _oldPath;
	root["newPath"] = _newPath;
	root["ipcAddr"] = _ipcAddr;
	return root.toStyledString();
}
