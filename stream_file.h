#pragma once

#include <string>

/*
 * The class is defined to save video stream into a file which name passed from outside 
 * The class manager a circle-buffer and a thread function
 * Outside call wirteStream function to put stream into circle-buffer
 * thread-func write a chunk from circle-buffer to file once.
*/

class StreamFile
{
	typedef struct ExtBuf {
		char* _buf;
		int _size;
	}ExtBuf;

public:
	StreamFile();
	~StreamFile();
	int writeStream(const char* stream, int len,const char* ipcUrl, const char* path, const char* fmt, double maxFileSize, const char* planId);
	bool isFileOpen() { return _isFileOpen;  }
	const char* fileName() { return _fileName.c_str(); }
private:
	int openFile(const char* name);
	void closeFile();
	int fileSize() { return ftell(_file); }
	void clear();
	void writeBytesToFile(const char*, int);
	bool createFileName();
	void publishFileName(double fileSize);
private:
	char* _writeBuf;
	int _length;
	int _writeBufSize;
	FILE * _file;
	bool _isFileOpen;
	std::string _fileName;
	std::string _ipcUrl;
	std::string _path;
	std::string _fmt;
	std::string _planId;
	double _maxFileSize;
};
