#include "pch.h"
#include "module_test.h"

#ifdef UNITTEST
#pragma warning(disable : 4018)

#include "../include/cppunit/ui/text/TestRunner.h"
#include "stream_file.h"

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ModuleTest, "ModuleTest");


void ModuleTest::setUp()
{
	int log_level = 5;
	_mqInf = new MqInf();
	_app = new SipApp(_mqInf);

	pj_init();
	pjlib_util_init();
	_log.init();
	pj_caching_pool_init(&_cp, &pj_pool_factory_default_policy, 0);
	_pool = pj_pool_create(&_cp.factory, "sipsvc", 1024, 1024, NULL);

	_app->init(&_cp, _pool);
	_mqInf->init(&_cp, _pool);
}

void ModuleTest::tearDown()
{
	_mqInf->unini();
	_app->unini();
	delete _mqInf;
	delete _app;

	if (_pool) {
		pj_pool_release(_pool);
	}
	pj_caching_pool_destroy(&_cp);
	pj_shutdown();
	_log.unini();
}

void ModuleTest::testSaveStream()
{
	int len = 255;
	char *buf = new char[len];
	int i;
	for (i = 0; i < len; i++) {
		buf[i] = i + 1;
	}

	StreamFile streamFile;
	int ret = streamFile.openFile("./testOut/testStream.ps", _pool);
	CPPUNIT_ASSERT_EQUAL(ret, 0);
	for (i = 0; i < 1000; i++) {
		streamFile.writeStream(buf, len);
		pj_thread_sleep(10);
	}
	int fsize = streamFile.fileSize();
	CPPUNIT_ASSERT_EQUAL(fsize, 1000 * len);
	streamFile.closeFile();
	FILE * file = fopen("./testOut/testStream.ps", "r+b");
	CPPUNIT_ASSERT(file);
	char* buf2 = new char[len];
	for (i = 0; i < 1000; i++) {
		int rdlen = 0, rdlen2 = 0;
		while (rdlen < len && !feof(file)) {
			rdlen2 = fread(buf2 + rdlen, 1, len - rdlen, file);
			if (rdlen2 > 0) {
				rdlen += rdlen2;
			}
		}
		for (int j = 0; j < len; j++) {
			CPPUNIT_ASSERT_EQUAL(buf[j], buf2[j]);
		}
	}
	fclose(file);
	delete [] buf2;
	delete[] buf;
}
#endif
