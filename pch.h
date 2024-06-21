// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件

#ifndef PCH_H
#define PCH_H

// TODO: 添加要在此处预编译的标头

#include <iostream>
#include <fstream>
#include <time.h>
#include <limits.h>
#include "tcmalloc.h"
#include "malloc_extension.h"
#include "pjproject.h"
#include "sipapp.h"
#include "mqinf.h"
#include "sip_log.h"
#include "stream_mgr.h"
#include "config_mgr.h"
#include "ipc_call.h"

#ifdef NEED_BOOST
#include "boost/property_tree/ptree.hpp" // NOLINT
#include "boost/property_tree/ini_parser.hpp" // NOLINT
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/regex.hpp>
#define MyRegEx boost::regex
#define MyMatch boost::cmatch 
#define MyRegSearch boost::regex_search
#define MyRegError boost::regex_error
using namespace boost::property_tree;
#else
#include <regex>
#define MyRegEx std::regex
#define MyMatch std::smatch 
#define MyRegSearch std::regex_search
#define MyRegError std::regex_error
#endif
#endif //PCH_H
