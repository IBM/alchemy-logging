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

// Local
#include "alog/logger.hpp"

// Standard
#include <chrono>
#include <codecvt>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

// Third Party
#include <boost/algorithm/string/join.hpp>

namespace logging
{

// Local Only //////////////////////////////////////////////////////////////////

namespace
{

// Static empty map data to allow reference semantics for non-empty maps
static const nlohmann::json s_emptyMapData;

// Implement string splitting to avoid boost dependency:
// CITE: http://stackoverflow.com/questions/236129/split-a-string-in-c
template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

detail::CLogChannelRegistrySingleton::FilterMap parseFilterSpec(const std::string& a_spec)
{
  // short-circuit for empty spec
  if (a_spec.empty()) return {{}};

  // Split on comma first
  const auto& splitVec = split(a_spec, ',');

  // Split on ':' and construct the output map
  detail::CLogChannelRegistrySingleton::FilterMap out;
  for (const auto& spec_pair : splitVec)
  {
    const auto& subSplitVec = split(spec_pair, ':');
    if (subSplitVec.size() != 2)
    {
      std::stringstream ss;
      ss << "Invalid Log Spec [" << a_spec << "]";
      throw std::runtime_error(ss.str());
    }
    out[subSplitVec[0]] = detail::ParseLevel(subSplitVec[1]);
  }
  return out;
}

// Get a timestamp formatted like 2018-04-17T21:42:11.583Z
std::string getTimestamp()
{
  const auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  char timestamp[20]; // Timestamp will always be 20 characters long
  // if strftime is unable to write the string to the buffer, it will return 0.
  if(std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", std::gmtime(&t)) == 0)
    return "ERROR_CREATING_TIMESTAMP";

  // Add the milliseconds
  const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  const auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(now);
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now_ms - now_s).count();
  std::stringstream ss;
  ss << timestamp << "." << std::setfill('0') << std::setw(3) << millis << "Z";
  return ss.str();
}

// CITE: https://stackoverflow.com/questions/15615136/is-codecvt-not-a-std-header
std::string wstring_to_utf8 (const std::wstring& str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.to_bytes(str);
}

// Helper to transform the keys of a map into a vector for initialization
std::vector<std::string> getMapKeys(const nlohmann::json& a_mdMap)
{
  std::vector<std::string> out;
  for (const auto& entry : a_mdMap.items())
  {
    out.push_back(entry.key());
  }
  return out;
}

// Pretty-printing for maps
void addPrettyPrintMap(
  const nlohmann::json& a_map,
  const std::string& a_format,
  const unsigned a_indent,
  const bool a_addNewlines,
  std::vector<std::string>& a_out)
{
  for (const auto& entry : a_map.items())
  {
    std::stringstream ss;
    ss << a_format;
    for (unsigned i = 0; i < a_indent; ++i)
    {
      ss << logging::detail::INDENT_VALUE;
    }
    ss << entry.key() << ": ";

    // If the value is a map, recurse with an extra level of indentation
    const auto val = entry.value();
    if (val.is_object())
    {
      std::vector<std::string> lines;
      addPrettyPrintMap(val, a_format, a_indent+1, false, lines);
      ss << "\n" + boost::algorithm::join(lines, "\n");
    }
    else
    {
      ss << val;
    }

    if (a_addNewlines)
    {
      ss << "\n";
    }

    a_out.push_back(ss.str());
  }
}


} // end anon namespace


// Types and Constants /////////////////////////////////////////////////////////

namespace detail
{

std::ostream& operator<<(std::ostream& a_stream, const detail::ELogLevels& a_level)
{
  static const char* strings[] =
  {
    "OFF ",
    "FATL",
    "ERRR",
    "WARN",
    "INFO",
    "TRCE",
    "DBUG",
    "DBG1",
    "DBG2",
    "DBG3",
    "DBG4",
  };

  if (static_cast< std::size_t >(a_level) < sizeof(strings) / sizeof(*strings))
    a_stream << strings[a_level];
  else
    a_stream << static_cast< int >(a_level);

  return a_stream;
}

std::string LevelToHumanString(const detail::ELogLevels& a_level)
{
  static const char* strings[] =
  {
    "off",
    "fatal",
    "error",
    "warning",
    "info",
    "trace",
    "debug",
    "debug1",
    "debug2",
    "debug3",
    "debug4",
  };

  if (static_cast< std::size_t >(a_level) < sizeof(strings) / sizeof(*strings))
    return strings[a_level];
  else
  {
    std::stringstream ss;
    ss << static_cast< int >(a_level);
    return ss.str();
  }
}

// Get a log level from a string
ELogLevels ParseLevel(const std::string& a_str)
{
  if      (a_str == "off")     return detail::ELogLevels::off;
  else if (a_str == "fatal")   return detail::ELogLevels::fatal;
  else if (a_str == "error")   return detail::ELogLevels::error;
  else if (a_str == "warning") return detail::ELogLevels::warning;
  else if (a_str == "info")    return detail::ELogLevels::info;
  else if (a_str == "trace")   return detail::ELogLevels::trace;
  else if (a_str == "debug")   return detail::ELogLevels::debug;
  else if (a_str == "debug1")  return detail::ELogLevels::debug1;
  else if (a_str == "debug2")  return detail::ELogLevels::debug2;
  else if (a_str == "debug3")  return detail::ELogLevels::debug3;
  else if (a_str == "debug4")  return detail::ELogLevels::debug4;
  else
  {
    std::stringstream ss;
    ss << "Invalid Log Level Spec [" << a_str << "]";
    throw std::runtime_error(ss.str());
    return detail::ELogLevels::error;
  }
}

CLogEntry::CLogEntry(const std::string& a_channel,
                     const ELogLevels a_level,
                     const std::string& a_message,
                     nlohmann::json a_mapData)
  : channel(a_channel),
    level(a_level),
    message(a_message),
    timestamp(getTimestamp()),
    serviceName(logging::detail::CLogChannelRegistrySingleton::instance()->getServiceName()),
    nIndent(logging::detail::CLogChannelRegistrySingleton::instance()->getIndent()),
    threadId(std::this_thread::get_id()),
    mapData(std::move(a_mapData))
{}

// CStdLogFormatter ////////////////////////////////////////////////////////////

std::vector<std::string> CStdLogFormatter::formatEntry(const CLogEntry& a_entry) const
{
  std::vector<std::string> out;

  // Split on newlines
  const auto& splitVec = split(a_entry.message, '\n');

  // Get the format string
  auto format = getHeader(a_entry);

  // NOTE: Filtering is done explicitly in the macro call to avoid the need to
  // allocate a stringstream if the filter fails.
  for (const auto& line : splitVec)
  {
    std::stringstream ss;
    ss << format << line << std::endl << std::flush;
    out.push_back(ss.str());
  }

  // Add map data lines
  if (not a_entry.mapData.empty())
  {
    addPrettyPrintMap(a_entry.mapData, format, 0, true, out);
  }

  return out;
}

std::string CStdLogFormatter::getHeader(const CLogEntry& a_entry) const
{
  std::stringstream ss;

  // Add the timestamp
  ss << a_entry.timestamp;

  // Add the service name if set
  if (not a_entry.serviceName.empty())
  {
    ss << " <" << a_entry.serviceName << ">";
  }

  // Add the channel and level
  std::string ch = a_entry.channel;
  ch.resize(logging::detail::MAX_CHANNEL_LENGTH, ' ');
  ss << " [" << ch << ":" << a_entry.level;
  if (logging::detail::CLogChannelRegistrySingleton::instance()->threadIDEnabled())
  {
    ss << ":" << a_entry.threadId;
  }
  ss << "] ";

  // Add the indent
  for (unsigned i = 0; i < a_entry.nIndent; ++i)
  {
    ss << INDENT_VALUE;
  }

  // Return
  return ss.str();
}

// CJSONLogFormatter ///////////////////////////////////////////////////////////

std::vector<std::string> CJSONLogFormatter::formatEntry(const CLogEntry& a_entry) const
{

  // Start with the arbitrary key/val map
  nlohmann::json j = a_entry.mapData;

  // Add standard fields
  j["channel"] = a_entry.channel;
  j["level_str"] = LevelToHumanString(a_entry.level);
  j["timestamp"] = a_entry.timestamp;
  j["num_indent"] = static_cast<int64_t>(a_entry.nIndent);

  // Add message if present
  if (not a_entry.message.empty())
  {
    j["message"] = a_entry.message;
  }

  // Add thread id if enabled
  if (logging::detail::CLogChannelRegistrySingleton::instance()->threadIDEnabled())
  {
    std::stringstream ss;
    ss << a_entry.threadId;
    j["thread_id"] = ss.str();
  }

  // Add service name if enabled
  if (not a_entry.serviceName.empty())
  {
    j["service_name"] = a_entry.serviceName;
  }

  // Serialize as output (pretty=false)
  return { j.dump() + "\n" };
}

// CLogChannelRegistrySingleton ////////////////////////////////////////////////

CLogChannelRegistrySingleton::Ptr CLogChannelRegistrySingleton::m_pInstance = {};

const CLogChannelRegistrySingleton::Ptr& CLogChannelRegistrySingleton::instance()
{
  if (not m_pInstance)
  {
    m_pInstance = CLogChannelRegistrySingleton::Ptr(new CLogChannelRegistrySingleton());
    m_pInstance->setFormatter(CLogFormatterBase::Ptr(new CStdLogFormatter()));
  }
  return m_pInstance;
}

void CLogChannelRegistrySingleton::setupFilters(const std::string& a_filterSpec,
                                                const std::string& a_defaultLevelSpec)
{
  TLock lock(m_mutex);
  m_filters = parseFilterSpec(a_filterSpec);
  m_defaultLevel = detail::ParseLevel(a_defaultLevelSpec);
}

void CLogChannelRegistrySingleton::addSink(std::basic_ostream<char>& a_sink)
{
  TLock lock(m_mutex);
  m_sinks.emplace_back(CSink(TStreamRef(a_sink)));
}

void CLogChannelRegistrySingleton::setFormatter(const CLogFormatterBase::Ptr& a_formatter)
{
  TLock lock(m_mutex);
  m_formatter = a_formatter;
}

void CLogChannelRegistrySingleton::enableThreadID()
{
  TLock lock(m_mutex);
  m_doThreadLog = true;
}

void CLogChannelRegistrySingleton::disableThreadID()
{
  TLock lock(m_mutex);
  m_doThreadLog = false;
}

void CLogChannelRegistrySingleton::enableMetadata()
{
  TLock lock(m_mutex);
  m_doMetadata = true;
}

void CLogChannelRegistrySingleton::disableMetadata()
{
  TLock lock(m_mutex);
  m_doMetadata = false;
}

void CLogChannelRegistrySingleton::setServiceName(const std::string& a_serviceName)
{
  TLock lock(m_mutex);
  m_serviceName = a_serviceName;
}

bool CLogChannelRegistrySingleton::filter(const std::string& a_channel,
                                          ELogLevels a_level) const
{
  if (a_level == logging::detail::ELogLevels::off) {\
    throw std::runtime_error("Logging to 'off' is not allowed");\
    return false;
  } else {
    auto iter = m_filters.find(a_channel);
    return (iter != m_filters.end() ? iter->second : m_defaultLevel) >= a_level;
  }
}

void CLogChannelRegistrySingleton::log(const std::string& a_channel,
                                       ELogLevels a_level,
                                       const std::string& a_msg,
                                       nlohmann::json a_mapData)
{
  // Make sure formatter is set
  if (not m_formatter)
  {
    return;
  }

  // Add metadata if present
  if (m_doMetadata)
  {
    const auto& tid = std::this_thread::get_id();
    const auto iter = m_metadata.find(tid);
    if (iter != m_metadata.end())
    {
      a_mapData["metadata"] = iter->second;
    }
  }

  // Create the CLogEntry instance
  CLogEntry entry(a_channel, a_level, a_msg, std::move(a_mapData));

  // Iterate through formatted strings
  for (const std::string& line : m_formatter->formatEntry(std::move(entry)))
  {
    // Log to each sink
    for (auto& sink : m_sinks)
    {
      TLock l_lock(*sink.m);
      sink.sink.get() << line << std::flush;
    }
  }
}

void CLogChannelRegistrySingleton::log(const std::string& a_channel,
                                       ELogLevels a_level,
                                       const std::wstring& a_msg,
                                       nlohmann::json a_mapData)
{
  log(a_channel, a_level, wstring_to_utf8(a_msg), a_mapData);
}

void CLogChannelRegistrySingleton::addIndent()
{
  TLock lock(m_mutex);
  const auto& tid = std::this_thread::get_id();
  auto iter = m_indents.find(tid);
  if (iter == m_indents.end())
  {
    m_indents.insert(iter, std::make_pair(tid, 1));
  }
  else
  {
    ++(iter->second);
  }
}

void CLogChannelRegistrySingleton::removeIndent()
{
  TLock lock(m_mutex);
  const auto& tid = std::this_thread::get_id();
  const auto iter = m_indents.find(tid);
  if (iter != m_indents.end() and iter->second > 0)
  {
    --(iter->second);
  }
  if (iter != m_indents.end() and iter->second == 0)
  {
    m_indents.erase(iter);
  }
}

unsigned CLogChannelRegistrySingleton::getIndent() const
{
  const auto& tid = std::this_thread::get_id();
  auto iter = m_indents.find(tid);
  if (iter == m_indents.end())
  {
    return 0;
  }
  else
  {
    return iter->second;
  }
}

void CLogChannelRegistrySingleton::addMetadata(const std::string& a_key,
                                               const nlohmann::json::value_type& a_value)
{
  if (m_doMetadata)
  {
    TLock lock(m_mutex);
    const auto& tid = std::this_thread::get_id();
    const auto iter = m_metadata.find(tid);
    if (iter == m_metadata.end())
    {
      nlohmann::json threadMD;
      threadMD[a_key] = a_value;
      m_metadata.insert(iter, std::make_pair(tid, threadMD));
    }
    else
    {
      iter->second[a_key] = a_value;
    }
  }
}

void CLogChannelRegistrySingleton::removeMetadata(const std::string& a_key)
{
  if (m_doMetadata)
  {
    TLock lock(m_mutex);
    const auto& tid = std::this_thread::get_id();
    const auto tidIter = m_metadata.find(tid);
    if (tidIter != m_metadata.end())
    {
      auto entryIter = tidIter->second.find(a_key);
      if (entryIter != tidIter->second.end())
      {
        tidIter->second.erase(entryIter);
        if (tidIter->second.empty())
        {
          m_metadata.erase(tidIter);
        }
      }
    }
  }
}

void CLogChannelRegistrySingleton::clearMetadata()
{
  if (m_doMetadata)
  {
    TLock lock(m_mutex);
    const auto tid = std::this_thread::get_id();
    const auto iter = m_metadata.find(tid);
    if (iter != m_metadata.end())
    {
      m_metadata.erase(iter);
    }
  }
}

const nlohmann::json&
CLogChannelRegistrySingleton::getMetadata() const
{
  const auto tid = std::this_thread::get_id();
  const auto iter = m_metadata.find(tid);
  if (iter != m_metadata.end())
  {
    return iter->second;
  }
  else
  {
    // Global singleton for empty metadata
    static const nlohmann::json s_emptyMetadata;
    return s_emptyMetadata;
  }
}

void CLogChannelRegistrySingleton::reset()
{
  TLock lock(m_mutex);
  m_sinks.clear();
  m_filters.clear();
  m_defaultLevel = ELogLevels::off;
  m_doThreadLog = false;
  m_serviceName = "";
  m_indents.clear();
  m_formatter = CLogFormatterBase::Ptr(new CStdLogFormatter());
}

// CLogScope ///////////////////////////////////////////////////////////////////

CLogScope::CLogScope(const std::string& a_channelName,
                   ELogLevels a_level,
                   const std::string& a_msg,
                   const TScopeLogMapPtr& a_mapDataPtr)
  : m_channelName(a_channelName),
    m_level(a_level),
    m_msg(a_msg),
    m_mapDataPtr(a_mapDataPtr)
{
  const nlohmann::json& l_mapData = m_mapDataPtr ? *m_mapDataPtr : s_emptyMapData;
  ALOG_LEVEL_IMPL(m_channelName, m_level, "Start: " << m_msg, l_mapData);
}

CLogScope::~CLogScope()
{
  const nlohmann::json& l_mapData = m_mapDataPtr ? *m_mapDataPtr : s_emptyMapData;
  ALOG_LEVEL_IMPL(m_channelName, m_level, "End: " << m_msg, l_mapData);
}

// CLogScopedTimer /////////////////////////////////////////////////////////////

CLogScopedTimer::CLogScopedTimer(const std::string& a_channelName,
                                 ELogLevels a_level,
                                 const std::string& a_msg,
                                 const TScopeLogMapPtr& a_mapDataPtr)
  : m_channelName(a_channelName),
    m_level(a_level),
    m_msg(a_msg),
    m_mapDataPtr(a_mapDataPtr),
    m_t0()
{
  if (logging::detail::CLogChannelRegistrySingleton::instance()->filter(m_channelName, m_level))
  {
    m_t0 = std::chrono::high_resolution_clock::now();
  }
}

unsigned long CLogScopedTimer::getCurrentDurationNS() const
{
  const auto t1 = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(t1-m_t0).count();
}

CLogScopedTimer::~CLogScopedTimer()
{
  if (logging::detail::CLogChannelRegistrySingleton::instance()->filter(m_channelName, m_level))
  {
    const auto t1 = std::chrono::high_resolution_clock::now();

    float val = 0;
    std::string suffix = "";

    // Start with ns
    val = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-m_t0).count();
    suffix = "ns";

    // [100000000] => seconds
    if (val >= 100000000)
    {
      val = std::chrono::duration_cast<std::chrono::seconds>(t1-m_t0).count();
      suffix = "s";
    }

    // [1000000] => milliseconds
    else if (val >= 1000000)
    {
      val = std::chrono::duration_cast<std::chrono::milliseconds>(t1-m_t0).count();
      suffix = "ms";
    }

    // [1000] => microseconds
    else if (val >= 1000)
    {
      val = std::chrono::duration_cast<std::chrono::microseconds>(t1-m_t0).count();
      suffix = "us";
    }

    // Stream the message
    std::stringstream ss;
    ss << m_msg << val << suffix;
    nlohmann::json mapOut;
    if (m_mapDataPtr)
    {
      mapOut = *m_mapDataPtr;
    }

    // Add the duration in milliseconds
    mapOut["duration_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(t1-m_t0).count();
    ALOG_LEVEL_IMPL(m_channelName, m_level, ss.str(), mapOut);
  }
}

// CLogScopedIndent ////////////////////////////////////////////////////////////

CLogScopedIndent::CLogScopedIndent() : m_enabled(true)
{
  CLogChannelRegistrySingleton::instance()->addIndent();
}

CLogScopedIndent::CLogScopedIndent(const std::string& a_channelName, ELogLevels a_level)
  : m_enabled(CLogChannelRegistrySingleton::instance()->filter(a_channelName, a_level))
{
  if (m_enabled)
  {
    CLogChannelRegistrySingleton::instance()->addIndent();
  }
}

CLogScopedIndent::~CLogScopedIndent()
{
  if (m_enabled)
  {
    CLogChannelRegistrySingleton::instance()->removeIndent();
  }
}

// CLogScopedMetadata //////////////////////////////////////////////////////////

CLogScopedMetadata::CLogScopedMetadata(const std::string& a_key,
                                       const nlohmann::json::value_type& a_val)
  : m_keys({a_key})
{
  CLogChannelRegistrySingleton::instance()->addMetadata(a_key, a_val);
}

CLogScopedMetadata::CLogScopedMetadata(const nlohmann::json& a_mdMap)
  : m_keys(getMapKeys(a_mdMap))
{
  for (const auto& entry : a_mdMap.items())
  {
    CLogChannelRegistrySingleton::instance()->addMetadata(entry.key(), entry.value());
  }
}

CLogScopedMetadata::~CLogScopedMetadata()
{
  for (const auto& key : m_keys)
  {
    CLogChannelRegistrySingleton::instance()->removeMetadata(key);
  }
}

// Init Functions //////////////////////////////////////////////////////////////

void InitLogStream(std::basic_ostream<char>& a_stream)
{
  CLogChannelRegistrySingleton::instance()->addSink(a_stream);
}

std::shared_ptr<std::ofstream> InitLogFile(const std::string& a_filename)
{
  auto out = std::shared_ptr<std::ofstream>(
    new std::ofstream(a_filename + ".log", std::ofstream::out));
  InitLogStream(*out);
  return out;
}

void UseStdFormatter()
{
  CLogChannelRegistrySingleton::instance()->setFormatter(
    CLogFormatterBase::Ptr(new CStdLogFormatter()));
}

void UseJSONFormatter()
{
  CLogChannelRegistrySingleton::instance()->setFormatter(
    CLogFormatterBase::Ptr(new CJSONLogFormatter()));
}

} // end namespace detail
} // end namespace logging

// Non-Namespace Public Functions //////////////////////////////////////////////

void ALOG_SETUP(
  const std::string& defaultLevel,
  const std::string& filterSpec)
{
  logging::detail::CLogChannelRegistrySingleton
    ::instance()->setupFilters(filterSpec, defaultLevel);
  logging::detail::InitLogStream(std::cout);
}

void ALOG_ADJUST_LEVELS(
  const std::string& defaultLevel,
  const std::string& filterSpec)
{
  logging::detail::CLogChannelRegistrySingleton
    ::instance()->setupFilters(filterSpec, defaultLevel);
}

void ALOG_ENABLE_THREAD_ID()
{
  logging::detail::CLogChannelRegistrySingleton::instance()->enableThreadID();
}

void ALOG_DISABLE_THREAD_ID()
{
  logging::detail::CLogChannelRegistrySingleton::instance()->disableThreadID();
}

void ALOG_ENABLE_METADATA()
{
  logging::detail::CLogChannelRegistrySingleton::instance()->enableMetadata();
}

void ALOG_DISABLE_METADATA()
{
  logging::detail::CLogChannelRegistrySingleton::instance()->disableMetadata();
}

void ALOG_SERVICE_NAME(const std::string& name)
{
  logging::detail::CLogChannelRegistrySingleton::instance()->setServiceName(name);
}

void ALOG_USE_STD_FORMATTER()
{
  logging::detail::UseStdFormatter();
}

void ALOG_USE_JSON_FORMATTER()
{
  logging::detail::UseJSONFormatter();
}

void ALOG_RESET()
{
  logging::detail::CLogChannelRegistrySingleton::instance()->reset();
}
