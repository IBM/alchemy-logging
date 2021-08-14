/*------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2021 IBM
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *----------------------------------------------------------------------------*/
#pragma once

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

#include "alog/logger.hpp"

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
