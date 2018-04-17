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
#include <vector>
#include <string>
#include <sstream>

// First Party
#include <logger.h>

// Local
#include "util.h"
#include "fibonacci.h"

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