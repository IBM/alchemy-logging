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


#include "unit_testing.h"

#include <regex>
#include <stdlib.h>
#include <functional>
#include <chrono>
#include <thread>

#include <boost/regex.hpp>

using json = nlohmann::json;

namespace test
{

// Helpers /////////////////////////////////////////////////////////////////////

// Count the number of lines in a file
unsigned countFileLines(const std::string& a_filename)
{
  std::ifstream f(a_filename.c_str(), std::ifstream::in);
  if (f.good())
  {
    return std::count(std::istreambuf_iterator<char>(f),
                      std::istreambuf_iterator<char>(),
                      '\n');
  }
  else
  {
    return 0;
  }
}

// Reverse-parse a level header string to a level enum
logging::detail::ELogLevels parseHeaderLevel(const std::string& a_str)
{
  if      (a_str == "OFF ") return logging::detail::ELogLevels::off;
  else if (a_str == "FATL") return logging::detail::ELogLevels::fatal;
  else if (a_str == "ERRR") return logging::detail::ELogLevels::error;
  else if (a_str == "WARN") return logging::detail::ELogLevels::warning;
  else if (a_str == "INFO") return logging::detail::ELogLevels::info;
  else if (a_str == "TRCE") return logging::detail::ELogLevels::trace;
  else if (a_str == "DBUG") return logging::detail::ELogLevels::debug;
  else if (a_str == "DBG1") return logging::detail::ELogLevels::debug1;
  else if (a_str == "DBG2") return logging::detail::ELogLevels::debug2;
  else if (a_str == "DBG3") return logging::detail::ELogLevels::debug3;
  else if (a_str == "DBG4") return logging::detail::ELogLevels::debug4;
  else
  {
    std::stringstream ss;
    ss << "Invalid Log Level Spec [" << a_str << "]";
    throw std::runtime_error(ss.str());
    return logging::detail::ELogLevels::error;
  }
}

// Struct to represent a parsed log entry
struct CParsedLogEntry
{
  std::string                 channel;
  logging::detail::ELogLevels level;
  std::string                 message;
  json                        mapData;
  std::string                 timestamp;
  std::string                 serviceName;
  unsigned                    nIndent;
  std::string                 threadId;

  // Default constructor
  CParsedLogEntry() = default;

  // "Expected entry" constructor
  CParsedLogEntry(const std::string& ch,
                  logging::detail::ELogLevels lvl,
                  const std::string& msg,
                  json md = {},
                  unsigned indt = 0,
                  const std::string& svcNm = "",
                  bool hasTID = false)
  : channel(ch),
    level(lvl),
    message(msg),
    mapData(std::move(md)),
    serviceName(svcNm),
    nIndent(indt),
    threadId(hasTID ? "present" : "")
  {}

};  // end CParsedLogEntry

// Parse a line of plain-text logging into a CParsedLogEntry (null if parse failed)
std::shared_ptr<CParsedLogEntry> parseStdLine(const std::string& a_line)
{
  // this regex also validates the format of the timestamp
  // example timestamp: 2018-04-22T11:36:44.215Z
  //                               2018  -   04   -   22   T   11   :   36   :   44   .  215   Z
  std::string timestampRegex = "([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3}Z)";
  std::string restOfRegex = "([^\\]]*)\\[([^:]*):([^\\]:]*):?([^\\]\\s]*)\\] ([\\s]*)([^\\s].*)\n?";
  std::stringstream ss;
  ss << "^" << timestampRegex << " " << restOfRegex << "$";
  boost::regex re(ss.str());
  boost::smatch m;
  boost::regex_match(a_line, m, re);
  if (m.size() != 8)
  {
    return {};
  }
  else
  {
    // Create a dummy output that we'll override
    std::shared_ptr<CParsedLogEntry> out(new CParsedLogEntry());

    // Parse timestamp
    out->timestamp = m[1];

    // Parse service name if there
    out->serviceName = m[2];
    if (not out->serviceName.empty())
    {
      boost::regex snRe("<([^>]*)> ");
      boost::smatch snM;
      boost::regex_match(out->serviceName, snM, snRe);
      if (snM.size() == 2)
      {
        out->serviceName = snM[1];
      }
    }

    // Parse channel
    out->channel = m[3];

    // Parse level
    out->level = parseHeaderLevel(m[4]);

    // Set the thread id (even if empty)
    out->threadId = m[5];

    // Determine the number of indents
    out->nIndent = 0;
    std::string rawIndent = m[6];
    for (size_t nPos = rawIndent.find(logging::detail::INDENT_VALUE, 0);
         nPos != std::string::npos;
         nPos = rawIndent.find(logging::detail::INDENT_VALUE, nPos+logging::detail::INDENT_VALUE.size()))
    {
      ++out->nIndent;
    }

    // Set the message
    out->message = m[7];

    // Return the populated entry
    return out;
  }
}

// Parse a line of json logging into a CParsedLogEntry (null if parse failed)
std::shared_ptr<CParsedLogEntry> parseJSONLine(const std::string& a_line)
{
  std::shared_ptr<CParsedLogEntry> out(new CParsedLogEntry());

  // Parse into json
  auto j = json::parse(a_line);

  // timestamp
  {
    auto iter = j.find("timestamp");
    if (iter == j.end())
    {
      std::cerr << "Failed to parse timestamp" << std::endl;
      return {};
    }
    else
    {
      out->timestamp = iter->template get<std::string>();
    }
  }

  // service name
  {
    auto iter = j.find("service_name");
    if (iter != j.end())
    {
      out->serviceName = iter->template get<std::string>();
    }
  }

  // channel
  {
    auto iter = j.find("channel");
    if (iter == j.end())
    {
      std::cerr << "Failed to parse channel" << std::endl;
      return {};
    }
    else
    {
      out->channel = iter->template get<std::string>();
    }
  }

  // level
  {
    auto iter = j.find("level_str");
    if (iter == j.end())
    {
      std::cerr << "Failed to parse level" << std::endl;
      return {};
    }
    else
    {
      out->level = logging::detail::ParseLevel(iter->template get<std::string>());
    }
  }

  // thread id
  {
    auto iter = j.find("thread_id");
    if (iter != j.end())
    {
      out->threadId = iter->template get<std::string>();
    }
  }

  // num indent
  {
    auto iter = j.find("num_indent");
    if (iter == j.end())
    {
      std::cerr << "Failed to parse num_indent" << std::endl;
      return {};
    }
    else
    {
      out->nIndent = static_cast<unsigned>(iter->template get<int64_t>());
    }
  }

  // message
  {
    auto iter = j.find("message");
    if (iter != j.end())
    {
      out->message = iter->template get<std::string>();
    }
  }

  // mapData
  out->mapData = j;
  for (const auto& key : std::vector<std::string>{
    "timestamp", "service_name", "channel", "level_str", "thread_id", "num_indent", "message"})
  {
    if (out->mapData.count(key))
    {
      out->mapData.erase(key);
    }
  }

  return out;
}

template<typename T>
bool compareVariantType(const json& a, const json& b)
{
  try
  {
    return a.get<T>() == b.get<T>();
  }
  catch (const json::type_error&)
  {
    return false;
  }
}
// Compare two parsed entries
bool entriesMatch(const CParsedLogEntry& exp,
                  const CParsedLogEntry& got,
                  bool checkMessage = true,
                  bool verbose = true)
{
  bool result = true;

  // channel
  if (exp.channel.compare(got.channel) != 0)
  {
    if (verbose) std::cerr << "Channel mismatch. Expected [" << exp.channel << "], Got [" << got.channel << "]" << std::endl;
    result = false;
  }

  // level
  if (exp.level != got.level)
  {
    if (verbose) std::cerr << "Level mismatch. Expected [" << exp.level << "], Got [" << got.level << "]" << std::endl;
    result = false;
  }

  // serviceName
  if (exp.serviceName.compare(got.serviceName) != 0)
  {
    if (verbose) std::cerr << "Service name mismatch. Expected [" << exp.serviceName << "], Got [" << got.serviceName << "]" << std::endl;
    result = false;
  }

  // message
  if (checkMessage and exp.message.compare(got.message) != 0)
  {
    if (verbose) std::cerr << "Message mismatch. Expected [" << exp.message << "], Got [" << got.message << "]" << std::endl;
    result = false;
  }

  // nIndent
  if (exp.nIndent != got.nIndent)
  {
    if (verbose) std::cerr << "Indent mismatch. Expected [" << exp.nIndent << "], Got [" << got.nIndent << "]" << std::endl;
    result = false;
  }

  // timestamp - make sure it's not empty
  if (got.timestamp.empty())
  {
    if (verbose) std::cerr << "Got empty timestamp" << std::endl;
    result = false;
  }

  // thread id - Make sure present or not
  if (exp.threadId.empty() and not got.threadId.empty())
  {
    if (verbose) std::cerr << "Got thread Id when none expected" << std::endl;
    result = false;
  }
  else if (not exp.threadId.empty() and got.threadId.empty())
  {
    if (verbose) std::cerr << "Missing thread Id when expected one" << std::endl;
    result = false;
  }

  // mapData
  if (exp.mapData.size() != got.mapData.size())
  {
    if (verbose)
    {
      std::cerr << "Size mismatch for map data. Got " << got.mapData.size()
        << ", expected " << exp.mapData.size() << std::endl;
    }
    result = false;
  }
  for (const auto eEntry : exp.mapData.items())
  {
    const auto gIter = got.mapData.find(eEntry.key());
    if (gIter == got.mapData.end())
    {
      if (verbose) std::cerr << "Missing expected mapData key [" << eEntry.key() << "]" << std::endl;
      result = false;
    }
    else if (checkMessage and *gIter != eEntry.value())
    {
      if (verbose)
      {
        std::cerr << "Value mismatch for mapData key [" << *gIter << "] != [" << eEntry.value() << "]" << std::endl;
      }
      result = false;
    }
  }
  for (const auto gEntry : got.mapData.items())
  {
    const auto eIter = exp.mapData.find(gEntry.key());
    if (eIter == exp.mapData.end())
    {
      if (verbose) std::cerr << "Got unexpected mapData key [" << gEntry.key() << "]" << std::endl;
      result = false;
    }
  }

  if (verbose and not result)
  {
    std::cerr << "----" << std::endl;
  }
  return result;
}

typedef std::function<std::shared_ptr<CParsedLogEntry>(const std::string&)> TLogParseFn;

// Verify a set of expected logging lines
bool verifyLinesImpl(TLogParseFn parse,
                     const std::string& logString,
                     const std::vector<CParsedLogEntry>& expEntries,
                     bool checkMessage,
                     bool unordered)
{
  // Split the log string into lines
  std::vector<std::string> lines;
  std::stringstream ss(logString);
  std::string line;
  while (std::getline(ss, line, '\n'))
  {
    lines.push_back(line);
    line = "";
  }

  // Make sure the expected number of lines were found
  if (lines.size() != expEntries.size())
  {
    std::cerr << "Size mismatch. Expected " << expEntries.size() << ", Got " << lines.size() << std::endl;
    std::cerr << logString;
    return false;
  }

  // Verify each line
  bool result = true;
  for (size_t i = 0; i < expEntries.size(); ++i)
  {
    // Parse the line
    const auto& got = parse(lines[i]);
    if (not got)
    {
      std::cerr << "Failed to parse log line [" << i << "]" << std::endl;
      result = false;
    }
    // Compare the two entries if ordered
    else if (not unordered)
    {
      result = entriesMatch(expEntries[i], *got, checkMessage) and result;
    }
    // Look for a match if unordered
    else
    {
      bool foundMatch = false;
      for (const auto& expEntry : expEntries)
      {
        if (entriesMatch(expEntry, *got, checkMessage, true))
        {
          foundMatch = true;
          break;
        }
      }
      if (not foundMatch)
      {
        std::cerr << "No match found for entry " << i << std::endl;
      }
      result = foundMatch and result;
    }
  }

  return result;
}

// Verify a set of expected logging lines as Std formatted
bool verifyStdLines(const std::string& logString,
                    const std::vector<CParsedLogEntry>& expEntries,
                    bool checkMessage = true,
                    bool unordered = false)
{
  return verifyLinesImpl(parseStdLine, logString, expEntries, checkMessage, unordered);
}

// Verify a set of expected logging lines as JSON formatted
bool verifyJSONLines(const std::string& logString,
                     const std::vector<CParsedLogEntry>& expEntries,
                     bool checkMessage = true,
                     bool unordered = false)
{
  return verifyLinesImpl(parseJSONLine, logString, expEntries, checkMessage, unordered);
}

// Sample class that is logging enabled
class CLoggingClassTest
{
public:

  ALOG_USE_CHANNEL(TEST)
  CLoggingClassTest() {}

  void doit() const
  {
    ALOGthis(info, "Some interesting information: " << 1 << ", " << 2);
  }

  void loggedFn() const
  {
    ALOG_FUNCTIONthis("");
    ALOGthis(info, "Some logging within a class...");
  }
};

void loggedFn()
{
  ALOG_FUNCTION(TEST, 1 << " testing...");
  ALOG(TEST, info, "Some logging...");
}

void loggedMapFn()
{
  std::shared_ptr<json> mapPtr(new json());
  ALOG_FUNCTION(TEST, 1 << " testing...", mapPtr);
  (*mapPtr)["foo"] = "bar";
  ALOG(TEST, info, "Some logging...");
}

json jsonExample1()
{
  json j;
  j["string_key"] = std::string("foo");
  j["int_key"] = 1l;
  j["bool_key"] = true;
  j["double_key"] = -3.1415;
  j["null_key"] = nullptr;
  return j;
}

json jsonExample2()
{
  json j;
  j["foo"] = std::string("bar");
  j["baz"] = {1l, 2l, 3l};
  j["bat"] = json{
    {"buz", "biz"},
    {"first", int64_t(2)}
  };
  return j;
}

// Tests ///////////////////////////////////////////////////////////////////////
// Test Suite Wrapper
ALCHEMY_TEST_SUITE(CAlogTest);

using namespace logging::detail;

////////
// Test writing to a log stream using a logging enabled class
////////
TEST_F(CAlogTest, LoggingClassStream)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("", "debug4");
  InitLogStream(ss);

  CLoggingClassTest tester;
  tester.doit();

  // Verify the file has one line
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::info, "Some interesting information: 1, 2"),
  }));
}

////////
// Test writing to a log stream with filters
////////
TEST_F(CAlogTest, LoggingFilterStream)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);

  // Log a line on TEST that is at the filter level
  // => YES
  std::string line1 = "Line on TEST at debug";
  ALOG(TEST, debug, line1);

  // Log a line on TEST that is below the filter level
  // => YES
  std::string line2 = "Line on TEST at info";
  ALOG(TEST, info, line2);

  // Log a line on TEST that is above the filter level
  // => NO
  std::string line3 = "Line on TEST at debug4";
  ALOG(TEST, debug4, line3);

  // Log a line on FOO that is at the filter level
  // => YES
  std::string line4 = "Line on FOO at info";
  ALOG(FOO, info, line4);

  // Log a line on BAR that is at the filter level
  // => NO
  std::string line5 = "Line on BAR at info";
  ALOG(BAR, info, line5);

  // Verify the results
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::debug, line1),
    CParsedLogEntry("TEST ", ELogLevels::info, line2),
    CParsedLogEntry("FOO  ", ELogLevels::info, line4),
  }));
  std::cout << ss.str() << std::endl;
}

////////
// Test the default level for unspecified channels
////////
TEST_F(CAlogTest, LoggingDefaultLevel)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:error", "info");
  InitLogStream(ss);

  // Log a line on BAR that is at the default level
  // => YES
  std::string line1 = "Line on BAR at info";
  ALOG(BAR, info, line1);

  // Log a line on BAR that is above the default level
  // => NO
  std::string line2 = "Line on BAR at debug2";
  ALOG(BAR, debug2, line2);

  // Log a line on FOO that is below default but above spec
  // => NO
  std::string line3 = "Line on FOO at warning";
  ALOG(FOO, warning, line3);

  // Log a line on FOO that is below default but at spec
  // => YES
  std::string line4 = "Line on FOO at error";
  ALOG(FOO, error, line4);

  // Verify the number of lines
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("BAR  ", ELogLevels::info, line1),
    CParsedLogEntry("FOO  ", ELogLevels::error, line4),
  }));
  std::cout << ss.str() << std::endl;
}

////////
// Test log lines with messages and map data
////////
TEST_F(CAlogTest, LoggingMsgAndMap)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:error", "info");
  InitLogStream(ss);

  // Log a line on BAR with both a message and key/val map
  std::string line1 = "Line on BAR at info";
  ALOG(BAR, info, line1, (json{{"foo", 123}}));

  // Verify the number of lines
  CParsedLogEntry entry1("BAR  ", ELogLevels::info, line1);
  CParsedLogEntry entry2("BAR  ", ELogLevels::info, "foo: 123");
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{entry1, entry2}));
}

////////
// Test that off is not a valid log channel
////////
TEST_F(CAlogTest, LoggingOff)
{
  std::stringstream ss;
  InitLogStream(ss);

  // Logging to off should throw
  try
  {
    ALOG(BAR, off, "Invalid line on off");
    ASSERT_TRUE(false);
  }
  catch(const std::runtime_error&)
  {
    ASSERT_TRUE(true);
  }

  // Verify the number of lines
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{}));
}

////////
// Test ALOG_SCOPED_BLOCK
////////
TEST_F(CAlogTest, LogScope)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);

  // Start/end lines should be added at the beginning and end
  {
    ALOG_SCOPED_BLOCK(TEST, debug, "Testing " << 1 << " with streaming");
    ALOG(TEST, debug, "Interim logging!");
  }

  // Verify the number of lines
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::debug, "Start: Testing 1 with streaming"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "Interim logging!"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "End: Testing 1 with streaming"),
  }));
}

////////
// Test ALOG_SCOPED_BLOCK with map data
////////
TEST_F(CAlogTest, LogScopeWithMap)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);

  // Start/end lines with map data that changes between start and end
  {
    // Set up a scope log with a mutable key/val map
    std::shared_ptr<json> map(new json());
    (*map)["foo"] = "bar";
    ALOG_SCOPED_BLOCK(TEST, debug, "Test with map", map);

    // Update the content of the map before the scope closes
    (*map)["foo"] = "baz";
    (*map)["buz"] = 123;
  }

  // Verify the number of lines
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{

    // Start
    CParsedLogEntry("TEST ", ELogLevels::debug, "Start: Test with map"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "foo: \"bar\""),

    // End
    CParsedLogEntry("TEST ", ELogLevels::debug, "End: Test with map"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "foo: \"baz\""),
    CParsedLogEntry("TEST ", ELogLevels::debug, "buz: 123"),
  }, true, true));
}

////////
// Test indentation
////////
TEST_F(CAlogTest, indentation)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);

  // Declare the lines for later comparison
  std::string line0 = "First line";
  std::string line1 = "Indented line";
  std::string line2 = "Doubly indented!";
  std::string line3 = "Singly indented...";
  std::string line4 = "Fully unwound!";

  // Log a non-indented line
  ALOG(TEST, info, line0);

  // Add an indented scope with logging
  {
    ALOG_SCOPED_INDENT();
    ALOG(TEST, info, line1);
    {
      ALOG_SCOPED_INDENT();
      ALOG(TEST, info, line2);
    }
    ALOG(TEST, info, line3);
  }
  ALOG(TEST, info, line4);

  // Verify the number of lines
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::info, line0, {}, 0),
    CParsedLogEntry("TEST ", ELogLevels::info, line1, {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::info, line2, {}, 2),
    CParsedLogEntry("TEST ", ELogLevels::info, line3, {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::info, line4, {}, 0),
  }));
}

////////
// Test ALOG_FUNCTION
////////
TEST_F(CAlogTest, FunctionBlock)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);

  // Test free function
  loggedFn();

  // Test with a class
  CLoggingClassTest tester;
  tester.loggedFn();

  // Verify the result (without checking message content)
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::trace, "", {}, 0),
    CParsedLogEntry("TEST ", ELogLevels::info, "", {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::trace, "", {}, 0),
    CParsedLogEntry("TEST ", ELogLevels::trace, "", {}, 0),
    CParsedLogEntry("TEST ", ELogLevels::info, "", {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::trace, "", {}, 0),
  }, false));
}

////////
// Test ALOG_FUNCTION with a map
////////
TEST_F(CAlogTest, FunctionBlockWithMap)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);

  // Test free function
  loggedMapFn();

  // Verify the result
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::trace, "Start: loggedMapFn( 1 testing... )", {}, 0),
    CParsedLogEntry("TEST ", ELogLevels::info, "Some logging...", {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::trace, "End: loggedMapFn( 1 testing... )", {}, 0),
    CParsedLogEntry("TEST ", ELogLevels::trace, "foo: \"bar\"", {}, 0),
  }));
}

////////
// Test ALOG_SERVICE_NAME
////////
TEST_F(CAlogTest, ServiceName)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);
  std::string svcName = "test_service";
  CLogChannelRegistrySingleton::instance()->setServiceName(svcName);

  // Log a line
  std::string line1 = "This is a test";
  ALOG(TEST, info, line1);

  // Verify the result
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::info, line1, {}, 0, svcName),
  }));
}

////////
// Test ALOG_MAP
////////
TEST_F(CAlogTest, Map)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);

  // Log two map data lines
  auto j1 = jsonExample1();
  auto j2 = jsonExample2();
  ALOG(TEST, info, "Hi there BEFORE a map");
  ALOG_MAP(TEST, info, j1);
  ALOG_MAP(TEST, info, j2);
  ALOG(TEST, info, "Hi there AFTER a map");

  // Verify the result
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::info, "Hi there BEFORE a map"),

    // j1
    CParsedLogEntry("TEST ", ELogLevels::info, "string_key: \"foo\""),
    CParsedLogEntry("TEST ", ELogLevels::info, "int_key: 1"),
    CParsedLogEntry("TEST ", ELogLevels::info, "bool_key: true"),
    CParsedLogEntry("TEST ", ELogLevels::info, "null_key: null"),
    CParsedLogEntry("TEST ", ELogLevels::info, "double_key: -3.1415"),

    // j2
    CParsedLogEntry("TEST ", ELogLevels::info, "foo: \"bar\""),
    CParsedLogEntry("TEST ", ELogLevels::info, "baz: [1,2,3]"),
    CParsedLogEntry("TEST ", ELogLevels::info, "bat: "),
    CParsedLogEntry("TEST ", ELogLevels::info, "buz: \"biz\"", {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::info, "first: 2", {}, 1),

    CParsedLogEntry("TEST ", ELogLevels::info, "Hi there AFTER a map"),
  }, true, true));
}

////////
// Test ALOG_SCOPED_METADATA
////////
TEST_F(CAlogTest, ScopedMetadata)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);
  CLogChannelRegistrySingleton::instance()->enableMetadata();

  // Outer scope
  {
    ALOG_SCOPED_METADATA("foo", "string_val");
    ALOG(TEST, debug, "Line with outer metadata BEFORE");

    // Inner scope
    {
      ALOG_SCOPED_METADATA("bar", 123);
      ALOG(FOO, info, "Line with inner metadata");
    }
    ALOG(TEST, debug, "Line with outer metadata AFTER");
  }
  ALOG(TEST, info, "Line with no metadata");

  // Verify results at given levels
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{

    // Outer scope BEFORE
    CParsedLogEntry("TEST ", ELogLevels::debug, "Line with outer metadata BEFORE"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "metadata: "),
    CParsedLogEntry("TEST ", ELogLevels::debug, "foo: \"string_val\"", {}, 1),

    // Inner scope
    CParsedLogEntry("FOO  ", ELogLevels::info, "Line with inner metadata"),
    CParsedLogEntry("FOO  ", ELogLevels::info, "metadata: "),
    CParsedLogEntry("FOO  ", ELogLevels::info, "foo: \"string_val\"", {}, 1),
    CParsedLogEntry("FOO  ", ELogLevels::info, "bar: 123", {}, 1),

    // Outer scope AFTER
    CParsedLogEntry("TEST ", ELogLevels::debug, "Line with outer metadata AFTER"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "metadata: "),
    CParsedLogEntry("TEST ", ELogLevels::debug, "foo: \"string_val\"", {}, 1),

    // Final line
    CParsedLogEntry("TEST ", ELogLevels::info, "Line with no metadata")
  }, true, true));
}

////////
// Test ALOG_SCOPED_METADATA with key/val map
////////
TEST_F(CAlogTest, ScopedMetadataMap)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);
  CLogChannelRegistrySingleton::instance()->enableMetadata();

  // Outer scope
  {
    ALOG_SCOPED_METADATA((json{
      {"foo", "string_val"},
      {"bar", 456}
    }));
    ALOG(TEST, debug, "Line with metadata map");
  }
  ALOG(TEST, info, "Line with no metadata");

  // Verify results at given levels
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{

    // Inside scope
    CParsedLogEntry("TEST ", ELogLevels::debug, "Line with metadata map"),
    CParsedLogEntry("TEST ", ELogLevels::debug, "metadata: "),
    CParsedLogEntry("TEST ", ELogLevels::debug, "foo: \"string_val\"", {}, 1),
    CParsedLogEntry("TEST ", ELogLevels::debug, "bar: 456", {}, 1),

    // Outside scope
    CParsedLogEntry("TEST ", ELogLevels::info, "Line with no metadata")
  }, true, true));
}

////////
// Test that multiple scope objects can be created in the same scope
// NOTE: If this compiles, it passes since the point is to ensure unique names
////////
TEST_F(CAlogTest, MultiScope)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);

  {
    ALOG_SCOPED_INDENT();
    ALOG_SCOPED_INDENT();
    ALOG_SCOPED_INDENT_IF(TEST, info);
    ALOG_SCOPED_INDENT_IF(FOO, info);
    ALOG_SCOPED_BLOCK(TEST, info, "Scoped block 1");
    ALOG_SCOPED_BLOCK(FOO, info, "Scoped block 2");
    ALOG_SCOPED_TIMER(TEST, info, "Scoped timer 1");
    ALOG_SCOPED_TIMER(FOO, info, "Scoped timer 2");
    ALOG_SCOPED_METADATA("foo", "bar");
    ALOG_SCOPED_METADATA("baz", "bat");
  }
}

////////
// Test ALOG_ADJUST_LEVELS
////////
TEST_F(CAlogTest, AdjustLevels)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);

  // Log a line on TEST that is at the filter level
  // => YES
  std::string line1 = "Line on TEST at debug";
  ALOG(TEST, debug, line1);

  // Log a line on FOO that is above the filter level
  // => NO
  std::string line2 = "Line on FOO at debug4";
  ALOG(FOO, debug4, line2);

  // Log a line on BAR that is above the default level
  // => NO
  std::string line3 = "Line on BAR at warning";
  ALOG(BAR, warning, line3);

  // Verify the results
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST ", ELogLevels::debug, line1),
  }));
  std::cout << ss.str() << std::endl;
  ss.str("");

  // Adjust the levels
  ALOG_ADJUST_LEVELS("warning", "FOO:debug4,TEST:off");

  // Log a line on TEST that is filtered out
  // => NO
  ALOG(TEST, debug, line1);

  // Log a line on FOO that is enabled with new filter
  // => YES
  ALOG(FOO, debug4, line2);

  // Log a line on BAR that is at the new default level
  // => YES
  ALOG(BAR, warning, line3);

  // Verify the results
  EXPECT_TRUE(verifyStdLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("FOO  ", ELogLevels::debug4, line2),
    CParsedLogEntry("BAR  ", ELogLevels::warning, line3),
  }));
  std::cout << ss.str() << std::endl;
}

////////
// Test ALOGW
////////
TEST_F(CAlogTest, WideChar)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("", "debug");
  InitLogStream(ss);

  // Log a line on TEST that is at the filter level
  // => YES
  std::wstring line1 = L"ﺏﺍﺭﺎﻛ ﺃﻮﺑﺎﻣﺍ ﺮﺌﻴﺳﺍ ﻞﻟﻭﻼﻳﺎﺗ ﺎﻠﻤﺘﺣﺩﺓ";
  ALOGW(TEST, debug, line1);

  // Verify that it was logged
  std::string result = ss.str();
  std::cout << result << std::endl;
  EXPECT_FALSE(result.empty());
}

////////
// Test ALOG_NEW_SCOPED_TIMER
////////
TEST_F(CAlogTest, NewScopedTimer)
{
  ALOG_SETUP("", true, "debug", "");

  // Scope with a timer that can be queried
  {
    const auto timer = ALOG_NEW_SCOPED_TIMER(TEST, debug, "Scope done in: ");
    ALOG(TEST, debug, "Starting scope");

    // Sleep for two milliseconds and make sure the delta registers >= 2ns
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const unsigned long dt1 = timer.getCurrentDurationNS();
    ALOG(TEST, debug, "First time delta: " << dt1 << "ns");
    EXPECT_GE(dt1, 2 * 1000000);

    // Sleep for two more milliseconds and make sure the delta registers >= 4ns
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const unsigned long dt2 = timer.getCurrentDurationNS();
    ALOG(TEST, debug, "Second time delta: " << dt2 << "ns");
    EXPECT_GE(dt2, 4 * 1000000);
  }
}

//// JSON Tests ////////////////////////////////////////////////////////////////

////////
// Test ALOG_USE_JSON_FORMATTER
////////
TEST_F(CAlogTest, JSONFormatter)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);
  UseJSONFormatter();

  // Log two lines
  std::string line1 = "This is a test";
  std::string line2 = "This is a second test";
  ALOG(TEST, info, line1);
  ALOG(TEST, info, line2);

  // Indentation lines
  std::string blockText = "This is a block";
  std::string warningText = "An indented warning";
  {
    ALOG_SCOPED_BLOCK(TEST, info, blockText);
    ALOG_SCOPED_INDENT();
    ALOG(TEST, warning, warningText);
  }

  // Verify the result
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyJSONLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST", ELogLevels::info, line1, {}, 0),
    CParsedLogEntry("TEST", ELogLevels::info, line2, {}, 0),
    CParsedLogEntry("TEST", ELogLevels::info, "Start: " + blockText, {}, 0),
    CParsedLogEntry("TEST", ELogLevels::warning, warningText, {}, 1),
    CParsedLogEntry("TEST", ELogLevels::info, "End: " + blockText, {}, 0),
  }));
}

////////
// Test ALOG_USE_JSON_FORMATTER with service name and thread id
////////
TEST_F(CAlogTest, JSONFormatterServiceNameAndThreadID)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);
  UseJSONFormatter();
  CLogChannelRegistrySingleton::instance()->enableThreadID();
  std::string serviceName = "test_service";
  CLogChannelRegistrySingleton::instance()->setServiceName(serviceName);

  // Log two lines
  std::string line1 = "This is a test";
  std::string line2 = "This is a second test";
  ALOG(TEST, info, line1);
  ALOG(TEST, info, line2);

  // Verify the result
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyJSONLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST", ELogLevels::info, line1, {}, 0, serviceName, true),
    CParsedLogEntry("TEST", ELogLevels::info, line2, {}, 0, serviceName, true),
  }));
}

////////
// Test ALOG_USE_JSON_FORMATTER with arbitrary map data
////////
TEST_F(CAlogTest, JSONFormatterMapData)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "info");
  InitLogStream(ss);
  UseJSONFormatter();

  // Log two map data lines
  auto j1 = jsonExample1();
  auto j2 = jsonExample2();
  ALOG_MAP(TEST, info, j1);
  ALOG_MAP(TEST, info, j2);

  // Verify the result
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyJSONLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("TEST", ELogLevels::info, "", j1),
    CParsedLogEntry("TEST", ELogLevels::info, "", j2),
  }));
}

////////
// Test ALOG_USE_JSON_FORMATTER log lines with messages and map data
////////
TEST_F(CAlogTest, JSONLoggingMsgAndMap)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:error", "info");
  InitLogStream(ss);
  UseJSONFormatter();

  // Log a line on BAR with both a message and key/val map
  std::string line1 = "Line on BAR at info";
  json map {
    {"foo", 123},
    {"bar", "baz"}
  };
  ALOG(BAR, info, line1, map);

  // Verify the number of lines
  EXPECT_TRUE(verifyJSONLines(ss.str(), std::vector<CParsedLogEntry>{
    CParsedLogEntry("BAR", ELogLevels::info, line1, map),
  }));
  std::cout << ss.str() << std::endl;
}

////////
// Test ALOG_SCOPED_METADATA with json formatting
////////
TEST_F(CAlogTest, JSONScopedMetadata)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);
  CLogChannelRegistrySingleton::instance()->enableMetadata();
  UseJSONFormatter();

  // Outer scope
  {
    ALOG_SCOPED_METADATA("foo", "string_val");
    ALOG(TEST, debug, "Line with outer metadata BEFORE");

    // Inner scope
    {
      ALOG_SCOPED_METADATA("bar", 123);
      ALOG(FOO, info, "Line with inner metadata");
    }
    ALOG(TEST, debug, "Line with outer metadata AFTER");
  }
  ALOG(TEST, info, "Line with no metadata");

  // Verify results at given levels
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyJSONLines(ss.str(), std::vector<CParsedLogEntry>{

    // Outer scope BEFORE
    CParsedLogEntry("TEST", ELogLevels::debug, "Line with outer metadata BEFORE",
      {{"metadata", {{"foo", "string_val"}}}}),

    // Inner scope
    CParsedLogEntry("FOO", ELogLevels::info, "Line with inner metadata",
      {{"metadata", {{"foo", "string_val"}, {"bar", 123}}}}),

    // Outer scope AFTER
    CParsedLogEntry("TEST", ELogLevels::debug, "Line with outer metadata AFTER",
      {{"metadata", {{"foo", "string_val"}}}}),

    // Final line
    CParsedLogEntry("TEST", ELogLevels::info, "Line with no metadata")
  }, true, false));
}

////////
// Test ALOG_SCOPED_TIMER with json formatting
////////
TEST_F(CAlogTest, JSONScopedTimer)
{
  std::stringstream ss;
  CLogChannelRegistrySingleton::instance()->setupFilters("TEST:debug,FOO:info", "off");
  InitLogStream(ss);
  UseJSONFormatter();

  // Outer scope
  {
    ALOG_SCOPED_TIMER(TEST, info, "Outer Block Completed in: ");
    // Inner Scope with map data
    {
      std::shared_ptr<json> mapDataPtr(new json());
      (*mapDataPtr)["mutable"] = "A";
      ALOG_SCOPED_TIMER(TEST, debug, "Inner block with map data and a stream " << 123, mapDataPtr);

      ALOG(FOO, info, "Hi from FOO");
      (*mapDataPtr)["added_later"] = 456;
      (*mapDataPtr)["mutable"] = "B";
    }
  }

  // Verify results at given levels (ignore messages due to time values)
  std::cout << ss.str() << std::endl;
  EXPECT_TRUE(verifyJSONLines(ss.str(), std::vector<CParsedLogEntry>{

    // Inner scope log line
    CParsedLogEntry("FOO", ELogLevels::info, ""),

    // Inner scope timer completion
    CParsedLogEntry("TEST", ELogLevels::debug, "", (json{
      {"mutable", "B"},
      {"added_later", 456},
      {"duration_ms", 0}
    })),

    // Outer scope timer completion
    CParsedLogEntry("TEST", ELogLevels::info, "", (json{{"duration_ms", 0}})),
  }, false, true));
}

} // end namespace test
