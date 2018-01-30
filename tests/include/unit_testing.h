/*  *
 * IBM Confidential
 * OCO Source Materials
 *
 * 5737-C06
 * (C) Copyright IBM Corp. 2017 All Rights Reserved.
 *
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.

 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.

 *
 *  */

#pragma once

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

#include "logger.h"

namespace test
{

class CAlchemyTestBase : public ::testing::Test
{
protected:
  CAlchemyTestBase() {}
  virtual ~CAlchemyTestBase()
  {
    ALOG_RESET();
  }
};

#define ALCHEMY_TEST_SUITE(name) class name : public test::CAlchemyTestBase{}

#define SETTABLE_ARG(type, name, val) \
  type m_ ## name = val; \
  auto name(type v) -> decltype(*this) { m_ ## name = v; return *this; } \

#define BEGIN_STEP_COUNT() unsigned ___step = 0;
#define LOG_STEP() \
  ALOG(TEST, debug, "-------- STEP " << ___step++ << " --------");

} // end namespace test
