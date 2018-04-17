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

// Standard
#include <chrono>

// Local
#include "fibonacci.h"

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
  std::shared_ptr<jsonparser::TObject> timerMap(new jsonparser::TObject());
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
    // The ALOGthis macro logs a single line on the given channel/level pair. //
    // In this case, we use debug3 since this is a tight-loop log statement.  //
    // Also, since we want to log some details about the state of the loop,   //
    // we can use a key/value map. Note that for the inline-map definition,   //
    // we need an extra set of parens so avoid confusing the preprocessor.    //
    ////////////////////////////////////////////////////////////////////////////
    ALOGthis(debug3, "Running iteration " << c, (jsonparser::TObject{
      std::make_pair("first", ALOG_MAP_VALUE(first)),
      std::make_pair("second", ALOG_MAP_VALUE(second)),
      std::make_pair("next", ALOG_MAP_VALUE(next)),
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
  // Here we add a key to the timer map that will be logged at completion. To //
  // add the value, we use ALOG_MAP_VALUE to convert to the necessary map     //
  // value type.                                                              //
  //////////////////////////////////////////////////////////////////////////////
  timerMap->insert(std::make_pair("sequence_length", ALOG_MAP_VALUE(out.size())));

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
  for (auto& future : m_futures)
  {
    out.push_back(future.get());
  }
  return out;
}

} // end namespace fib
