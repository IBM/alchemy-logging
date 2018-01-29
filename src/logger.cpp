// (C) Copyright IBM Corp. 2015
/* 
 * File description: logger.cpp
 * Author information: Gabe Hart ghart@us.ibm.com
 */
#include "utils/logger.h"

#include <utility>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/date_time.hpp>

namespace logging
{

// Local Only /////////////////////////////////////////////////////////////////

namespace
{

// Get a log level from a string
detail::ELogLevels parseLevel(const std::string& a_str)
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

detail::CLogChannelRegistrySingleton::FilterMap parseFilterSpec(const std::string& a_spec)
{
  // short-circuit for empty spec
  if (a_spec.empty()) return {};

  typedef std::vector<std::string> split_vector_type;

  // Split on comma first
  split_vector_type splitVec;
  boost::algorithm::split(splitVec, a_spec, boost::algorithm::is_any_of(","));

  // Split on ':' and construct the output map
  detail::CLogChannelRegistrySingleton::FilterMap out;
  for (const auto& spec_pair : splitVec)
  {
    split_vector_type subSplitVec;
    boost::algorithm::split(subSplitVec, spec_pair, boost::algorithm::is_any_of(":"));
    if (subSplitVec.size() != 2)
    {
      std::stringstream ss;
      ss << "Invalid Log Spec [" << a_spec << "]";
      throw std::runtime_error(ss.str());
    }
    out[subSplitVec[0]] = parseLevel(subSplitVec[1]);
  }
  return out;
}

} // end anon namespace


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

// CLogChannelRegistrySingleton ///////////////////////////////////////////////

CLogChannelRegistrySingleton::Ptr CLogChannelRegistrySingleton::m_pInstance = {};

const CLogChannelRegistrySingleton::Ptr& CLogChannelRegistrySingleton::instance()
{
  if (not m_pInstance)
    m_pInstance = CLogChannelRegistrySingleton::Ptr(new CLogChannelRegistrySingleton());
  return m_pInstance;
}

void CLogChannelRegistrySingleton::setupFilters(const std::string& a_filterSpec,
                                                const std::string& a_defaultLevelSpec)
{
  TLock lock(m_mutex);
  m_filters = parseFilterSpec(a_filterSpec);
  m_defaultLevel = parseLevel(a_defaultLevelSpec);
}

void CLogChannelRegistrySingleton::addSink(std::basic_ostream<char>& a_sink)
{
  TLock lock(m_mutex);
  m_sinks.push_back(TStreamRef(a_sink));
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

bool CLogChannelRegistrySingleton::filter(const std::string& a_channel,
                                          ELogLevels a_level) const
{
  auto iter = m_filters.find(a_channel);
  return (iter != m_filters.end() ? iter->second : m_defaultLevel) >= a_level;
}

void CLogChannelRegistrySingleton::log(const std::string& a_channel,
                                       ELogLevels a_level,
                                       const std::string& a_msg)
{
  // Split on newlines
  std::vector<std::string> splitVec;
  boost::algorithm::split(splitVec, a_msg, boost::algorithm::is_any_of("\r\n"));

  // Get the format string
  auto format = getFormatting(a_channel, a_level);

  // NOTE: Filtering is done explicitly in the macro call to avoid the need to
  // allocate a stringstream if the filter fails.
  TLock lock(m_mutex);
  for (auto& sink : m_sinks)
  {
    for (const auto& line : splitVec)
    {
      sink.get() << format << line << std::endl << std::flush;
    }
  }
}

void CLogChannelRegistrySingleton::addIndent()
{
  TLock lock(m_mutex);
  auto tid = boost::this_thread::get_id();
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
  auto tid = boost::this_thread::get_id();
  auto iter = m_indents.find(tid);
  if (iter != m_indents.end() and iter->second > 0)
  {
    --(iter->second);
  }
}

void CLogChannelRegistrySingleton::reset()
{
  TLock lock(m_mutex);
  m_sinks.clear();
  m_filters.clear();
  m_defaultLevel = ELogLevels::off;
  m_doThreadLog = false;
  m_indents.clear();
}

std::string CLogChannelRegistrySingleton::getFormatting(std::string a_channel,
                                                        ELogLevels a_level) const
{
  // Add the timestamp
  boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
  static std::locale loc(std::cout.getloc(), new boost::posix_time::time_facet("%Y%m%d_%H%M%S"));
  std::stringstream timeSS;
  timeSS.imbue(loc);
  timeSS << now;

  // Add the channel and level
  std::stringstream ss;
  a_channel.resize(logging::detail::MAX_CHANNEL_LENGTH, ' ');
  ss << "[" << timeSS.str() << " ";
  if (m_doThreadLog) ss << boost::this_thread::get_id() << " ";
  ss << a_channel << ": " << a_level << "] ";

  // Add the indent
  const auto iter = m_indents.find(boost::this_thread::get_id());
  if (iter != m_indents.end())
  {
    for (unsigned i = 0; i < iter->second; ++i)
    {
      ss << INDENT_VALUE;
    }
  }

  // Return
  return ss.str();
}

// CLogScope //////////////////////////////////////////////////////////////////

CLogScope::CLogScope(const std::string& a_logName,
                   ELogLevels a_level,
                   const std::string& a_msg)
  : m_logName(a_logName),
    m_level(a_level),
    m_msg(a_msg)
{
  ALOG_LEVEL_IMPL(m_logName, m_level, "Start: " << m_msg);
}

CLogScope::~CLogScope()
{
  ALOG_LEVEL_IMPL(m_logName, m_level, "End: " << m_msg);
}

// CLogScopedTimer /////////////////////////////////////////////////////////////

CLogScopedTimer::CLogScopedTimer(const std::string& a_logName,
                                 ELogLevels a_level,
                                 const std::string& a_msg)
  : m_logName(a_logName),
    m_level(a_level),
    m_msg(a_msg),
    m_t0()
{
  if (logging::detail::CLogChannelRegistrySingleton::instance()->filter(m_logName, m_level))
  {
    m_t0 = std::chrono::high_resolution_clock::now();
  }
}

CLogScopedTimer::~CLogScopedTimer()
{
  if (logging::detail::CLogChannelRegistrySingleton::instance()->filter(m_logName, m_level))
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
      val = std::chrono::duration<float, std::ratio<1,1>>(t1-m_t0).count();
      suffix = "s";
    }

    // [1000000] => milliseconds
    else if (val >= 1000000)
    {
      val = std::chrono::duration<float, std::ratio<1,1000>>(t1-m_t0).count();
      suffix = "ms";
    }

    // [1000] => microseconds
    else if (val >= 1000)
    {
      val = std::chrono::duration<float, std::ratio<1,1000000>>(t1-m_t0).count();
      suffix = "us";
    }

    // Stream the message
    std::stringstream ss;
    ss << m_msg << val << suffix;
    ALOG_LEVEL_IMPL(m_logName, m_level, ss.str());
  }
}
// CLogScopedIndent ////////////////////////////////////////////////////////////

CLogScopedIndent::CLogScopedIndent()
{
  CLogChannelRegistrySingleton::instance()->addIndent();
}

CLogScopedIndent::~CLogScopedIndent()
{
  CLogChannelRegistrySingleton::instance()->removeIndent();
}

// Init Functions /////////////////////////////////////////////////////////////

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

} // end namespace detail
} // end namespace logging
