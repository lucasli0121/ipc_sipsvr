/*
* stream_file.cpp 
* Copyright [2019] <liguoqiang>"
*/

#include "pch.h"  // NOLINT
#include "stream_file.h" // NOLINT
#include "sip_log.h" // NOLINT

static const char* THIS_FILE = "stream_file.cpp";

StreamFile::StreamFile()
{
	_writeBufSize = ConfigMgr::ins()->getFileBufSize();
	if (_writeBufSize <= 0) {
		_writeBufSize = 32000;
	}
	_writeBuf = new char[_writeBufSize];;
	_file = NULL;
	_isFileOpen = false;
	_length = 0;
	_maxFileSize = 0.0;
}

StreamFile::~StreamFile()
{
	TRACE_FUNC("StreamFile::~StreamFile()");
	clear();
	delete[]_writeBuf;
}

void StreamFile::clear()
{
	if (_length > 0) {
		writeBytesToFile(_writeBuf, _length);
		_length = 0;
	}
	closeFile();
}

void StreamFile::writeBytesToFile(const char* buf, int len)
{
	if(!_isFileOpen) {
		if(createFileName()) {
			openFile( _fileName.c_str());
		}
	}
	if(_isFileOpen) {
		while (len > 0) {
			size_t ret = fwrite(buf, sizeof(char), len, _file);
			len -= ret;
			buf += ret;
		}
		fflush(_file);
		if (_maxFileSize > 0.0) {
			double maxSize = _maxFileSize * 1024 * 1024; //
			double size = (double)fileSize();
			if (size >= maxSize) {
				closeFile();
				publishFileName(size);
			}
		}
	} else {
		SIP_LOG(SIP_ERROR, (THIS_FILE, "cann't open file, filename:%s", _fileName.c_str()));
	}
}

/*
*  append stream to circle buffer first
*  while if buffer is full 
*/
int StreamFile::writeStream(const char* stream, int streamlen, const char* ipcUrl, const char* path, const char* fmt, double maxFileSize, const char* planId)
{
	if (_ipcUrl.length() > 0 && _ipcUrl.compare(ipcUrl) != 0) {
		clear();
	}
	_ipcUrl = ipcUrl;
	if (_path.length() > 0 && _path.compare(path) != 0) {
		clear();
	}
 	_path = path;
	_fmt = fmt;	
	_maxFileSize = maxFileSize;
	_planId = planId;
	if (stream && streamlen > 0) {
		int remain = _writeBufSize - _length;
		if (streamlen <= remain) {
			memcpy(_writeBuf + _length, stream, streamlen);
			_length += streamlen;
		} else {
			writeBytesToFile(_writeBuf, _length);
			_length = 0;
			if (streamlen > _writeBufSize) {
				writeBytesToFile(stream, streamlen);
			} else {
				memcpy(_writeBuf, stream, streamlen);
				_length = streamlen;
			}
		}
	}
	return 0;
}

bool StreamFile::createFileName()
{
	TRACE_FUNC("StreamFile::createFileName()");
        std::string fileName = _path;
        char val[255];
        std::string ext("ps");
        if (_fmt.length() == 0) {
                ext = "ps";
        } else {
                ext = _fmt;
        }
        std::string filePrefix = _ipcUrl;
        std::replace(filePrefix.begin(), filePrefix.end(), ':', '_');
        time_t t;
        time(&t);
        struct tm* ptm = localtime(&t);
        sprintf(val,
                "%s_%4d%02d%02d%02d%02d%02d.%s",
                filePrefix.c_str(),
                ptm->tm_year + 1900,
                ptm->tm_mon + 1,
                ptm->tm_mday,
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                ext.c_str());
        fileName.append("/");
        fileName.append(val);
        _fileName = fileName;
	return true;
}
/*
 * open file
*/
int StreamFile::openFile(const char* name)
{
	TRACE_FUNC("StreamFile::openFile()");
	if (name == NULL) {
		return -1;
	}
	int ret = 0;
	_fileName = name;
	if (!_isFileOpen) {
		_isFileOpen = (_file = fopen(name, "w+b"));
	}
	return _isFileOpen ? 0 : -1;
}

void StreamFile::closeFile()
{
	TRACE_FUNC("StreamFile::closeFile()");
	if (_isFileOpen) {
		_isFileOpen = false;
		if (_file) {
			fclose(_file);
			_file = NULL;
		}
	}
}

void StreamFile::publishFileName(double fileSize)
{
	FileNameRpt obj;
    char val[255];
    memset(val, 0, sizeof(val));
    if (_ipcUrl.length() > 0 && _fileName.length() > 0) {
		obj.setPlanId(_planId.c_str());
		obj.setIpcAddr(_ipcUrl.c_str());
		obj.setFilePath(_path.c_str());
		obj.setFileName(_fileName.c_str());
		obj.setFileSize(fileSize);
		MqInf::ins()->publishMsg(FILE_NAME_REQ, &obj);
    }
}
