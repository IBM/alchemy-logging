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

// Standard
#include <chrono>

// Third Party
#include <nlohmann/json.hpp>

// Local
#include "fibonacci.h"

using json = nlohmann::json;

namespace fib
{

////////////////////////////////////////////////////////////////////////////////
// TUTORIAL:                                                                  //
//                                                                            //
// The ALOG_USE_CHANNEL_FREE macro defines a free-function "getChannel()"     //
// which returns the given channel name.                                      //
//                                                                            //
// WARNING: This MUST be called inside a namespace inside a .cpp file to      //
// avoid leaking the definition of getChannel() beyond the intended scope     //
////////////////////////////////////////////////////////////////////////////////
ALOG_USE_CHANNEL_FREE(LFIB)

TFibSequence fib(const unsigned n)
{
  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // The ALOG_DETAIL_FUNCTIONthis macro creates a Start/End scope with the    //
  // name of the current function on the given channel/level.                 //
  //////////////////////////////////////////////////////////////////////////////
  ALOG_DETAIL_FUNCTIONthis(debug, n);

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // For heavy-lifting or long-running functions, we often want to keep track //
  // of timing information. In addition, we often want to add information     //
  // about the results of the function to the timing block. To do so, we      //
  // create a map pointer that we give to the timer that we can populate at   //
  // any point before the scope closes.                                       //
  //////////////////////////////////////////////////////////////////////////////
  std::shared_ptr<json> timerMap(new json());
  ALOG_SCOPED_TIMERthis(debug, "Computed sequence of length " << n << " in ", timerMap);

  unsigned first = 0;
  unsigned second = 1;
  unsigned next = 0;
  TFibSequence out;

  for ( unsigned c = 0 ; c < n ; ++c )
  {
    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // The ALOG_MAPthis macro logs a key/value map on the given channel/level //
    // pair. In this case, we use debug3 since this is a tight-loop log       //
    // statement.                                                             //
    ////////////////////////////////////////////////////////////////////////////
    ALOG_MAPthis(debug3, (json{
      {"c", c},
      {"first", first},
      {"second", second},
      {"next", next},
    }));

    if ( c <= 1 )
       next = c;
    else
    {
       next = first + second;
       first = second;
       second = next;
    }
    // Simulate this being expensive
    std::this_thread::sleep_for(std::chrono::milliseconds(next));
    out.push_back(next);
  }

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // The ALOGthis macro takes an optional final argument to add a key/value   //
  // map to the log entry.                                                    //
  ////////////////////////////////////////////////////////////////////////////
  ALOGthis(debug3, "Final variable state", (json{
    {"first", first},
    {"second", second},
    {"next", next},
  }));

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // Here we add a key to the timer map that will be logged at completion. To //
  // add the value, we use ALOG_MAP_VALUE to convert to the necessary map     //
  // value type.                                                              //
  //////////////////////////////////////////////////////////////////////////////
  (*timerMap)["sequence_length"] = out.size();

  return out;
}

// FibonacciCalculator /////////////////////////////////////////////////////////

void FibonacciCalculator::add_sequence_length(const unsigned n)
{
  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // We can add metadata values to any scope that will then be logged with    //
  // all entries that get created within the scope.                           //
  //////////////////////////////////////////////////////////////////////////////
  ALOG_SCOPED_METADATA("job_number", m_futures.size() + 1);

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // Top-level interface functions use ALOG_FUNCTIONthis to add Start/End     //
  // function logs on the trace level                                         //
  //////////////////////////////////////////////////////////////////////////////
  ALOG_FUNCTIONthis(n);
  m_futures.push_back(std::async(&fib, n));
}

std::vector<TFibSequence> FibonacciCalculator::get_results()
{
  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // When ther are no arguments to a function, you must provide an empty      //
  // string argument to ALOG_FUNCTIONthis since variadic macros don't support //
  // 0 or more arguments (only 1 or more)                                     //
  //////////////////////////////////////////////////////////////////////////////
  ALOG_FUNCTIONthis("");
  ALOG_SCOPED_TIMERthis(info, "Finished all jobs in ");

  std::vector<TFibSequence> out;
  unsigned futureNum = 0;
  for (auto& future : m_futures)
  {
    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // The ALOGthis macro logs a single line on the given channel/level pair. //
    // In this case, we use debug2 since this is a detail level log, but not  //
    // one that will produce overly verbose output.                           //
    ////////////////////////////////////////////////////////////////////////////
    ALOGthis(debug2, "Waiting on future " << ++futureNum);
    out.push_back(future.get());
  }
  return out;
}

} // end namespace fib
