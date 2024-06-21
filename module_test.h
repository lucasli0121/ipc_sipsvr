#pragma once

#ifdef UNITTEST

#include "../include/cppunit/extensions/HelperMacros.h"
#include "pjproject.h"

class ModuleTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ModuleTest);
	CPPUNIT_TEST(testSaveStream);
	CPPUNIT_TEST_SUITE_END();
public:
	virtual void setUp();
	virtual void tearDown();

	void testSaveStream();
private:
	pj_caching_pool _cp;
	pj_pool_t *_pool;
	SipLog _log;
	MqInf *_mqInf;
	SipApp *_app;
};

#endif