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
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <locale>
#include <future>
#include <chrono>

// First Party
#include <logger.h>

// Library /////////////////////////////////////////////////////////////////////
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

/// Type used for a fibonacci sequence
typedef std::vector<unsigned> TFibSequence;

/// Calculate the Fibonacci sequence to the given length
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
  void add_sequence_length(const unsigned n)
  {
    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // We can add metadata values to any scope that will then be logged with  //
    // all entries that get created within the scope.                         //
    ////////////////////////////////////////////////////////////////////////////
    ALOG_SCOPED_METADATA("job_number", m_futures.size() + 1);

    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // Top-level interface functions use ALOG_FUNCTIONthis to add Start/End   //
    // function logs on the trace level                                       //
    ////////////////////////////////////////////////////////////////////////////
    ALOG_FUNCTIONthis(n);
    m_futures.push_back(std::async(&fib, n));
  }

  std::vector<TFibSequence> get_results()
  {
    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // When ther are no arguments to a function, you must provide an empty    //
    // string argument to ALOG_FUNCTIONthis since variadic macros don't       //
    // support 0 or more arguments (only 1 or more)                           //
    ////////////////////////////////////////////////////////////////////////////
    ALOG_FUNCTIONthis("");
    ALOG_SCOPED_TIMERthis(info, "Finished all jobs in ");

    std::vector<TFibSequence> out;
    for (auto& future : m_futures)
    {
      out.push_back(future.get());
    }
    return out;
  }

private:

  std::vector<std::future<TFibSequence>> m_futures;
};

} // end namespace fib

namespace util
{

/// Convert a string to lowercase
std::string to_lower(const std::string& input)
{
  std::locale loc;
  std::stringstream ss;
  for (std::string::size_type i=0; i<input.length(); ++i)
    ss << std::tolower(input[i], loc);
  return ss.str();
}

/// Pull a string value from the environment
std::string load_env_string(const std::string& key, const std::string& default_val)
{
  std::string out = default_val;
  if (const char* env_val = std::getenv(key.c_str()))
  {
    out = std::string(env_val);
  }
  return out;
}

/// Pull a bool value from the environment
bool load_env_bool(const std::string& key, bool default_val)
{
  if (const char* env_val = std::getenv(key.c_str()))
  {
    std::string lower_val = to_lower(std::string(env_val));
    return lower_val == "true" or lower_val == "1";
  }
  return default_val;
}

} // end namespace util

// Main ////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Read configuration from environment
  const std::string log_file      = util::load_env_string("ALOG_FILE",             ""    );
  const std::string default_level = util::load_env_string("ALOG_DEFAULT_LEVEL",    "info");
  const std::string filters       = util::load_env_string("ALOG_FILTERS",          ""    );
  const bool log_to_screen        = util::load_env_bool(  "ALOG_LOG_TO_SCREEN",    true  );
  const bool use_json             = util::load_env_bool(  "ALOG_USE_JSON",         false );
  const bool enable_thread_id     = util::load_env_bool(  "ALOG_ENABLE_THREAD_ID", false );
  const bool enable_metadata      = util::load_env_bool(  "ALOG_ENABLE_METADATA",  false );

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // This demonstrates all of the standard configuration features of ALOG.    //
  // The configuration options are:                                           //
  //                                                                          //
  //   * Log file: If not empty, logs will be saved to the provided file      //
  //   * Log to screen: Print logs to stdout if true                          //
  //   * Default level: The level to enable for all channels not present in   //
  //       the filters                                                        //
  //   * Filters: Specific channel:level strings to change the enabled level  //
  //       for specific channels                                              //
  //   * Use JSON: If true, format logs as JSON rather than pretty-print      //
  //   * Thread ID: If true, all logs will contain the thread ID              //
  //   * Metadata: If true, metadata values will be added to each log entry   //
  //////////////////////////////////////////////////////////////////////////////
  ALOG_SETUP(log_file, log_to_screen, default_level, filters);
  if (use_json)
  {
    ALOG_USE_JSON_FORMATTER();
  }
  if (enable_thread_id)
  {
    ALOG_ENABLE_THREAD_ID();
  }
  if (enable_metadata)
  {
    ALOG_ENABLE_METADATA();
  }

  //////////////////////////////////////////////////////////////////////////////
  // TUTORIAL:                                                                //
  //                                                                          //
  // When logging with no configured channel, simply provide the channel as   //
  // the first argument to the non-this versions of the macros                //
  //////////////////////////////////////////////////////////////////////////////
  ALOG(MAIN, info, "Logging Configured");
  ALOG(MAIN, debug, "Hello World");

  // Parse command line args as numbers
  std::vector<unsigned> sequence_lengths;
  {
    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // When performing a logically grouped set of actions, it can be helpful  //
    // to have Start/End log blocks to wrap it. For this, use the             //
    // ALOG_SCOPED_BLOCK macro.                                               //
    ////////////////////////////////////////////////////////////////////////////
    ALOG_SCOPED_BLOCK(MAIN, debug, "Parsing Command Line");

    for (int i = 1; i < argc; ++i)
    {
      ALOG(MAIN, debug2, "Parsing argument " << i);
      try
      {
        int val = std::stoi(argv[i]);
        if (val < 0)
        {
          //////////////////////////////////////////////////////////////////////
          // TUTORIAL:                                                        //
          //                                                                  //
          // Only log to the `fatal` level when a fatal error has occurred    //
          // and the application is going down                                //
          //////////////////////////////////////////////////////////////////////
          ALOG(MAIN, fatal, "Invalid negative value [" << val << "]");
          return EXIT_FAILURE;
        }
        ALOG(MAIN, debug2, "Parsed value [" << val << "]");
        sequence_lengths.push_back(static_cast<unsigned>(val));
      }
      catch (const std::invalid_argument&)
      {
        ALOG(MAIN, fatal, "Invalid argument [" << argv[i] << "]");
        return EXIT_FAILURE;
      }
    }
    if (sequence_lengths.empty())
    {
      ALOG(MAIN, fatal, "Must provide at least one sequence length argument");
      return EXIT_FAILURE;
    }
  }

  // Create the calculator
  fib::FibonacciCalculator calculator;

  // For each provided number, compute the sequence using the calculator
  {
    ALOG_SCOPED_TIMER(MAIN, debug, "Done adding sequences in ");
    for (const auto length : sequence_lengths)
    {
      calculator.add_sequence_length(length);
    }
  }

  // Aggregate the results and log them
  const auto& results = calculator.get_results();
  for (const auto& sequence : results)
  {
    ////////////////////////////////////////////////////////////////////////////
    // TUTORIAL:                                                              //
    //                                                                        //
    // When constructing a logging string that requires more than a single    //
    // line, you can wrap the construction in ALOG_IS_ENABLED to avoid the    //
    // construction work if the channel/level that you will log to is not     //
    // enabled.                                                               //
    ////////////////////////////////////////////////////////////////////////////
    if (ALOG_IS_ENABLED(MAIN, info))
    {
      std::stringstream ss;
      ss << "[ ";
      for (const auto entry : sequence)
      {
        ss << entry << " ";
      }
      ss << "]";
      ALOG(MAIN, info, ss.str());
    }
  }
}