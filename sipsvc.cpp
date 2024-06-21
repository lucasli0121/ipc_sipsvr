// sipsvc.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// Copyright [2019] <liguoqiang>"

#include "pch.h" // NOLINT
#include "ps_codec.h" // NOLINT
#include "sip_log.h" // NOLINT
#include "stream_mgr.h" // NOLINT

static const char* THIS_FILE = "sipsvc";
static int mainLoop(void *arg);
static bool _hasRun = true;

#if defined(UNITTEST)

#include "cppunit/CompilerOutputter.h"
#include "cppunit/ui/text/TestRunner.h"
#include "cppunit/TestResult.h"
#include "cppunit/TestResultCollector.h"
#include "cppunit/TextOutputter.h"
#include "./module_test.h"

int main(int argc, char** argv)
{
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(ModuleTest::suite());

	std::ofstream fout("./sip_cppunit.txt");
	runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(&runner.result(),fout) );
	runner.run();

	return 0;
}

#else
#if defined(Linux) && defined(PJ_HAS_SEMAPHORE) && PJ_HAS_SEMAPHORE != 0
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
static sem_t * _quitLock = NULL;
static const char* sem_name = "sipSvcQuitLock";
#endif

int main(int argc, char** argv)
{
	pj_caching_pool cp;
	pj_pool_t *pool = NULL;
	pj_status_t status;

	const char* cfgFile = NULL;
	ConfigMgr _cfgMgr;
	bool needExit = false;
	if (argc < 3) {
		if(argc == 2) {
			if(strcmp(argv[1], "exit")  == 0) {
				needExit = true;
			}
		}
		if(!needExit) {
			printf("valid params: -c inifile");
			return 0;
		}
	}
#if defined(Linux) && defined(PJ_HAS_SEMAPHORE) && PJ_HAS_SEMAPHORE != 0
	if(needExit) {
		_quitLock = sem_open(sem_name, O_EXCL, S_IRUSR | S_IWUSR, 0);
		if(_quitLock != SEM_FAILED) {
			sem_post(_quitLock);
			sem_close(_quitLock);
		}
		return 0;
	} else {
		_quitLock = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 0);
	}
#endif
	if (strcmp(argv[1], "-c") == 0) {
		cfgFile = argv[2];
	}
	if (cfgFile == NULL) {
		printf("can't find config file, please input current params\n");
		return 0;
	}
	ptree iniTree;
	try {
		ini_parser::read_ini(cfgFile, iniTree);
	} catch (std::exception &e) {
		printf("cann't open file:%s\n", e.what());
		return 0;
	}
	/* read common all params from config-file*/
	try {
		basic_ptree<std::string, std::string> secItems = iniTree.get_child("common");
		int logsize = secItems.get<int>("logfile_size");
		_cfgMgr.setLogFileSize(logsize * 1024 * 1024);
	} catch (std::exception &e) {
		printf("read ini error:%s\n", e.what());
	}
	/* read sip all params from config-file*/
	try {
		basic_ptree<std::string, std::string> secItems = iniTree.get_child("sip");
		std::string host = secItems.get<std::string>("host");
		int port = secItems.get<int>("port");
		std::string id = secItems.get<std::string>("sipid");
		std::string channelId = secItems.get<std::string>("channelId");
		int auth = secItems.get<int>("register_auth");
		int useTcp = secItems.get<int>("tcp");
		int tmOut = secItems.get<int>("timeout");
		_cfgMgr.setSipHost(host.c_str());
		_cfgMgr.setSipPort(port);
		_cfgMgr.setSipId(id.c_str());
		_cfgMgr.setChannelId(channelId.c_str());
		_cfgMgr.setRegAuth(auth);
		_cfgMgr.setSipUseTcp(useTcp);
		_cfgMgr.setTimeout(tmOut);
	} catch (std::exception &e) {
		printf("read ini error:%s\n", e.what());
	}
	/* read rtp all params from config-file*/
	try {
		basic_ptree<std::string, std::string> secItems = iniTree.get_child("rtp");
		std::string rtpRecvIp = secItems.get<std::string>("rtpRecvIp");
		int startPort = secItems.get<int>("start_port");
		int portStep = secItems.get<int>("port_step");
		int usetcp = secItems.get<int>("tcp");
		int useudp = secItems.get<int>("udp");
		_cfgMgr.setRtpRecvIp(rtpRecvIp.c_str());
		_cfgMgr.setStartRtpPort(startPort);
		_cfgMgr.setRtpPortStep(portStep);
		_cfgMgr.setUseTcp(usetcp);
		_cfgMgr.setUseUdp(useudp);
	} catch (std::exception &e) {
		printf("read ini error:%s\n", e.what());
	}
	/* read video all params from config-file*/
	try {
		basic_ptree<std::string, std::string> secItems = iniTree.get_child("video");
		std::string fmt = secItems.get<std::string>("format");
		int frame = secItems.get<int>("frame");
		int bitrate = secItems.get<int>("bitrate");
		int clockrate = secItems.get<int>("clockrate");
		int forceKeyFrame = secItems.get<int>("force_keyframe");
		_cfgMgr.setVideoFmt(fmt.c_str());
		_cfgMgr.setFrame(frame);
		_cfgMgr.setBitRate(bitrate);
		_cfgMgr.setClockRate(clockrate);
		_cfgMgr.setForceKeyFrame(forceKeyFrame);
		
	} catch (std::exception &e) {
		printf("read ini error:%s\n", e.what());
	}
	/*read MQ all params from config-file*/
	try {
		basic_ptree<std::string, std::string> secItems = iniTree.get_child("mq");
		std::string mqAddress = secItems.get<std::string>("host");
		int mqPort = secItems.get<int>("port");
		int subqos = secItems.get<int>("sub_qos");
		int pubqos = secItems.get<int>("pub_qos");
		int msgCount = secItems.get<int>("message_count");
		int msgSize = secItems.get<int>("message_size");
		_cfgMgr.setMqServerAddr(mqAddress.c_str(), mqPort);
		_cfgMgr.setMqParams(subqos, pubqos, msgCount, msgSize);
	} catch (std::exception & e) {
		printf("read ini error:%s\n", e.what());
	}
	SipLog log;
	MqInf *mqInf = new MqInf();
	SipApp *app = new SipApp();
	StreamMgr * streamMgr = new StreamMgr();
	pj_init();
#ifdef _DEBUG
	pj_log_set_level(6);
#else
	pj_log_set_level(3);
#endif
	pjlib_util_init();
	pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);
	pool = pj_pool_create(&cp.factory, "sipsvc", 512, 512, NULL);
	log.init();
	status = pjmedia_video_format_mgr_create(pool, 64, 0, NULL);
	if (status != PJ_SUCCESS) {
		ERROR_LOG(THIS_FILE, "pjmedia_video_format_mgr_create failed", status);
		delete streamMgr;
		delete mqInf;
		delete app;
		pj_pool_release(pool);
		return -1;
	}

	status = pjmedia_converter_mgr_create(pool, NULL);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
	status = pjmedia_vid_codec_mgr_create(pool, NULL);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
	status = pjmedia_vid_dev_subsys_init(&cp.factory);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
	status = pjmedia_codec_ps_vid_init(NULL, &cp.factory);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);

#   if defined(PJMEDIA_HAS_OPENH264_CODEC) && PJMEDIA_HAS_OPENH264_CODEC != 0
	status = pjmedia_codec_openh264_vid_init(NULL, &cp.factory);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
#   endif

#  if defined(PJMEDIA_HAS_FFMPEG_VID_CODEC) && PJMEDIA_HAS_FFMPEG_VID_CODEC!=0
	/* Init ffmpeg video codecs */
	status = pjmedia_codec_ffmpeg_vid_init(NULL, &cp.factory);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, -1);
#  endif  /* PJMEDIA_HAS_FFMPEG_VID_CODEC */


	app->init(&cp);
	mqInf->init(pool);
	streamMgr->init(pool);
	status = app->start(argc, argv);
	if (status == PJ_SUCCESS) {
		printf("ipc sip server start ok!!\n");
	}
	pj_thread_t * thread = NULL;
	pj_thread_create(pool, "mainloop", mainLoop, NULL, 0, 0, &thread);
	while (_hasRun) {
		mqInf->loopMqMsg();
	}
	app->stop();
	app->unini();
	streamMgr->uninit();
	mqInf->unini();
	delete streamMgr;
	delete mqInf;
	delete app;

	pjmedia_codec_ps_vid_deinit();
#if defined(PJMEDIA_HAS_FFMPEG_VID_CODEC) && PJMEDIA_HAS_FFMPEG_VID_CODEC != 0
	pjmedia_codec_ffmpeg_vid_deinit();
#endif

#if defined(PJMEDIA_HAS_OPENH264_CODEC) && PJMEDIA_HAS_OPENH264_CODEC != 0
	pjmedia_codec_openh264_vid_deinit();
#endif

	log.unini();
	if (thread) {
		pj_thread_join(thread);
		pj_thread_destroy(thread);
		thread = NULL;
	}
#if defined(Linux) && defined(PJ_HAS_SEMAPHORE) && PJ_HAS_SEMAPHORE != 0
	if(_quitLock) {
		sem_close(_quitLock);
		sem_unlink(sem_name);
	}
#endif
	if (pool) {
		pj_pool_release(pool);
	}
	pj_caching_pool_destroy(&cp);
	pj_shutdown();
	return 0;
}
#endif

int mainLoop(void *arg)
{
#if defined(Linux) && defined(PJ_HAS_SEMAPHORE) && PJ_HAS_SEMAPHORE != 0
	sem_wait(_quitLock) ;
#else
	while(1) {
		char ch[255];
		scanf("%s", ch);
		if(strncmp(ch, "exit", 10) == 0) {
			break;
		}
	}
#endif
	_hasRun = false;
	return 0;
}
