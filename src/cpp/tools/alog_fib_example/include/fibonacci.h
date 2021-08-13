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

// Standard
#include <vector>
#include <future>

// First Party
#include <alog/logger.hpp>

namespace fib
{

/// Type used for a fibonacci sequence
typedef std::vector<unsigned> TFibSequence;

/// Calculate the Fibonacci sequence to the given length
TFibSequence fib(const unsigned n);

/// This class is responsible for creating threads that compute the fibonacci
/// sequence and aggregating the results.
class FibonacciCalculator
{
public:

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // The ALOG_USE_CHANNEL macro defines a member function "getChannel()" for  //
  // the enclosing class, allowing "this" flavor macros to be used in member. //
  // functions. It should be preferred over ALOG_USE_CHANNEL_FREE whenever    //
  // possible.                                                                //
  //////////////////////////////////////////////////////////////////////////////
  ALOG_USE_CHANNEL(FIB);

  /// Add a sequence length and start the sequence computation
  void add_sequence_length(const unsigned n);

  /// Wait for all jobs to complete and return the results
  std::vector<TFibSequence> get_results();

private:

  std::vector<std::future<TFibSequence>> m_futures;
};  // end class FibonacciCalculator

} // end namespace fib
