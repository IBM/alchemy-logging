/*  *
 * IBM Confidential
 * OCO Source Materials
 *
 * 5737-C06
 * (C) Copyright IBM Corp. 2018 All Rights Reserved.
 *
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has been
 * deposited with the U.S. Copyright Office.

 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.

 *
 *  */
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
