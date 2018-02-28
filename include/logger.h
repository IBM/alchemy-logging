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

/*
 *
 * File description: logger.h
 * Author information: Gabe Hart ghart@us.ibm.com
 */
#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <map>

#include <jsonparser/typedefs.h>

/* \brief This tool provides a thread-safe logging environment
 *
 * This logger tool provides standard logging functionality including multiple
 * channels and levels. In addition, it allows threaded applications to perform
 * logging operations concurrently while enforcing serialized write access to
 * the log itself.
 *
 * There are several key compiler definitions that can effect the behavior of
 * the logging system:
 *
 * DISABLE_LOGGING - Fully disable logging and complie it out
 * ALOG_SCOPED_FUNCTIONS - Use full scope for function names
 */

namespace logging
{
namespace detail
{

/*-- Types and Constants -----------------------------------------------------*/

/** The maximum length of a channel name when printed to the log. Note that
 * channel names may be longer than this in the code, but only this many
 * characters will appear in the log */
static const unsigned MAX_CHANNEL_LENGTH = 5;

/** This is the string used for a single indent */
static const std::string INDENT_VALUE = "  ";

/** \brief Custom severity level enum */
enum ELogLevels
{
  off = 0,
  fatal,
  error,
  warning,
  info,
  trace,
  debug,
  debug1,
  debug2,
  debug3,
  debug4
};
std::ostream& operator<<(std::ostream&, const detail::ELogLevels&);

/** \brief This struct encapsulates the full content of a log statement */
struct CLogEntry
{
  CLogEntry(const std::string& a_channel,
            const ELogLevels   a_level,
            const std::string& a_message,
            jsonparser::TObject a_mapData = {});
  std::string         channel;
  ELogLevels          level;
  std::string         message;
  std::string         timestamp;
  std::string         serviceName;
  unsigned            nIndent;
  std::thread::id     threadId;
  jsonparser::TObject mapData;
};  // end CLogEntry

/*-- Formatters --------------------------------------------------------------*/

/** \brief This class abstracts the process of formatting a log statement */
class CLogFormatterBase
{
public:

  typedef std::shared_ptr<CLogFormatterBase> Ptr;

  /** \brief The main function needed to format a log entry */
  virtual std::vector<std::string> formatEntry(const CLogEntry&) const = 0;

};  // end CLogFormatterBase

/** \brief Standard log formatter for easily readible logs */
class CStdLogFormatter : public CLogFormatterBase
{
public:
  virtual std::vector<std::string> formatEntry(const CLogEntry&) const override;
private:
  std::string getHeader(const CLogEntry&) const;
};  // end CStdLogFormatter

/** \brief JSON log formatter for structured log output */
class CJSONLogFormatter : public CLogFormatterBase
{
public:
  virtual std::vector<std::string> formatEntry(const CLogEntry&) const override;
};  // end CJSONLogFormatter

/*-- Core Singleton ----------------------------------------------------------*/

/** \brief This class is a singleton used to aggregate logging channels */
class CLogChannelRegistrySingleton
{
public:
  typedef std::shared_ptr<CLogChannelRegistrySingleton> Ptr;
  typedef std::unordered_map<std::string, ELogLevels> FilterMap;

  /** Allow access to the singleton instance */
  static const Ptr& instance();

  /** Set the filter levels and the default level from strings */
  void setupFilters(const std::string& a_filterSpec,
                    const std::string& a_defaultLevelSpec);

  /** Add a stream as a sink. It must stay in scope outside of this instance */
  void addSink(std::basic_ostream<char>& a_sink);

  /** Set the output formatter */
  void setFormatter(const CLogFormatterBase::Ptr&);

  /** Enable/disable thread id logging */
  void enableThreadID();
  void disableThreadID();

  /** Set the service name to use */
  void setServiceName(const std::string&);
  const std::string& getServiceName() const { return m_serviceName; }

  /** Determine whether ID threading is currently enabled */
  bool threadIDEnabled() const { return m_doThreadLog; }

  /** Filter based on the channel and level. This is public so that it can be
   * run before pushing the message content to a stringstream in the macro */
  bool filter(const std::string& a_channel, ELogLevels) const;

  /** Send the given string to all sinks with proper formatting. Filtering is
   * done before this is called in ALOG, so this function does no filtering. */
  void log(const std::string& a_channel,
           ELogLevels a_level,
           const std::string& a_msg,
           jsonparser::TObject a_mapData);

  /** Send the given string to all sinks with proper formatting. Filtering is
   * done before this is called in ALOG, so this function does no filtering. */
  void log(const std::string& a_channel,
           ELogLevels a_level,
           const std::wstring& a_msg,
           jsonparser::TObject a_mapData);

  /** Add a level of indentation for the current thread */
  void addIndent();

  /** Remove a level of indentation for the current thread */
  void removeIndent();

  /** Get the indent level for the current thread */
  unsigned getIndent() const;

  /** Add a key to the metadata for the current thread */
  void addMetadata(const std::string& a_key, const jsonparser::TJsonValue& a_value);

  /** Remove a key from the metadata for the current thread */
  void removeMetadata(const std::string& a_key);

  /** Clear the metadata for the current thread */
  void clearMetadata();

  /** Get a view into the current metadata dict for the current thread */
  const jsonparser::TObject& getMetadata() const;

  /** Clear the current filters and sinks and set the default level to off */
  void reset();

private:

  /* Private default constructor, deleted copy constructor and assignment
   * operator make this a singleton */
  CLogChannelRegistrySingleton()
    : m_defaultLevel(ELogLevels::off),
      m_doThreadLog(false)
  {};
  CLogChannelRegistrySingleton(const CLogChannelRegistrySingleton&) = delete;
  CLogChannelRegistrySingleton& operator=(const CLogChannelRegistrySingleton&) = delete;

  /* The global instance */
  static Ptr m_pInstance;

  FilterMap m_filters;
  ELogLevels m_defaultLevel;
  bool m_doThreadLog;
  std::string m_serviceName;

  typedef std::lock_guard<std::mutex> TLock;
  std::mutex m_mutex;

  typedef std::reference_wrapper<std::basic_ostream<char>> TStreamRef;
  struct CSink
  {
    TStreamRef sink;
    std::shared_ptr<std::mutex> m;
    CSink(const TStreamRef& s) : sink(s), m(new std::mutex()) {}
  };
  std::vector<CSink> m_sinks;
  CLogFormatterBase::Ptr m_formatter;

  typedef std::thread::id                          TThreadID;
  typedef std::map<TThreadID, unsigned>            ThreadIndentMap;
  typedef std::map<TThreadID, jsonparser::TObject> ThreadMetadataMap;
  ThreadIndentMap   m_indents;
  ThreadMetadataMap m_metadata;

};  // end class CLogChannelRegistrySingleton

/*-- Scope Classes -----------------------------------------------------------*/

/** \brief This class is used to add a Start/End block to a log */
class CLogScope
{
public:
  CLogScope(const CLogScope&) = delete;
  CLogScope(const std::string& a_logName,
            ELogLevels a_level,
            const std::string& a_msg);
  virtual ~CLogScope();
private:
  std::string m_logName;
  ELogLevels m_level;
  std::string m_msg;
};  // end class CLogScope

/** \brief Struct to time execution of a block */
struct CLogScopedTimer
{
  CLogScopedTimer(const CLogScopedTimer&) = delete;
  CLogScopedTimer(const std::string& a_logName,
                  ELogLevels a_level,
                  const std::string& a_msg);
  virtual ~CLogScopedTimer();
private:
  std::string m_logName;
  ELogLevels m_level;
  std::string m_msg;
  decltype(std::chrono::high_resolution_clock::now()) m_t0;
};  // end class CLogScopedTimer

/** \brief Struct to act as a proxy for logging indentation */
struct CLogScopedIndent
{
  CLogScopedIndent();
  CLogScopedIndent(const std::string& a_logName,
                   ELogLevels a_level);
  ~CLogScopedIndent();

private:
  const bool m_enabled;
};  // end class CLogScopedIndent

/** \brief Struct to scope metadata entries */
struct CLogScopedMetadata
{
  CLogScopedMetadata(const std::string&, const jsonparser::TJsonValue&);
  ~CLogScopedMetadata();
private:
  std::string m_key;
};  // end class CLogScopedMetadata

/** \brief Initiate a log stream */
void InitLogStream(std::basic_ostream<char>& a_stream);

/** \brief Open a file and return a shared_ptr to it */
std::shared_ptr<std::ofstream> InitLogFile(const std::string& a_filename);

/** \brief Use the standard log formatter */
void UseStdFormatter();

/** \brief Use the JSON log formatter */
void UseJSONFormatter();

/** \brief Get the human readible (lowercase, full length) level string */
std::string LevelToHumanString(const detail::ELogLevels&);

/** \brief Parse a log level from a string */
detail::ELogLevels ParseLevel(const std::string&);

/*-- Detail Helpers ----------------------------------------------------------*/

/** Helper for converting raw values to metadata */
template<typename T>
inline jsonparser::TJsonValue toMetadata(T v)
{ return jsonparser::TJsonValue(v); }

template<>
inline jsonparser::TJsonValue toMetadata(int v)
{ return jsonparser::TJsonValue(int64_t(v)); }

template<>
inline jsonparser::TJsonValue toMetadata(long v)
{ return jsonparser::TJsonValue(int64_t(v)); }

template<>
inline jsonparser::TJsonValue toMetadata(unsigned v)
{ return jsonparser::TJsonValue(int64_t(v)); }

template<>
inline jsonparser::TJsonValue toMetadata(unsigned long v)
{ return jsonparser::TJsonValue(int64_t(v)); }

template<>
inline jsonparser::TJsonValue toMetadata(const char* v)
{ return jsonparser::TJsonValue(std::string(v)); }

} // end namespace detail

} // end namespace logging

/*-- Detail Macros -----------------------------------------------------------*/

// CITE: https://stackoverflow.com/questions/1082192/how-to-generate-random-variable-names-in-c-using-macros/1082211
#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res
#define ALOG_UNIQUE_VAR_NAME_IMPL(base) PP_CAT(base, __LINE__)

#define ALOG_LEVEL_IMPL(channel, level, msg, map)\
  do {if (logging::detail::CLogChannelRegistrySingleton\
    ::instance()->filter(channel, level)) {\
    logging::detail::CLogChannelRegistrySingleton\
      ::instance()->log( channel, level,\
        static_cast<std::ostringstream&>(std::ostringstream().flush() << msg).str(), map);\
  }} while(0)

#define ALOGW_LEVEL_IMPL(channel, level, msg, map)\
  do {if (logging::detail::CLogChannelRegistrySingleton\
    ::instance()->filter(channel, level)) {\
    logging::detail::CLogChannelRegistrySingleton\
      ::instance()->log( channel, level,\
        static_cast<std::wostringstream&>(std::wostringstream().flush() << msg).str(), map);\
  }} while(0)

#define ALOG_CHANNEL_IMPL(channel, level, msg)\
  ALOG_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, msg, {})

#define ALOG_MAP_IMPL(channel, level, map)\
  ALOG_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, "", map)

#define ALOGW_CHANNEL_IMPL(channel, level, msg)\
  ALOGW_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, msg, {})

#define ALOG_SCOPED_BLOCK_IMPL(channel, level, msg)\
  logging::detail::CLogScope ALOG_UNIQUE_VAR_NAME_IMPL(_logScope) (\
    channel, logging::detail::ELogLevels:: level,\
    static_cast<std::ostringstream&>(std::ostringstream().flush() << msg).str())

#define ALOG_SCOPED_TIMER_IMPL(channel, level, msg)\
  logging::detail::CLogScopedTimer ALOG_UNIQUE_VAR_NAME_IMPL(_logTimer) (\
    channel, logging::detail::ELogLevels:: level,\
    static_cast<std::ostringstream&>(std::ostringstream().flush() << msg).str())

#define ALOG_SCOPED_METADATA_IMPL(key, value)\
  logging::detail::CLogScopedMetadata ALOG_UNIQUE_VAR_NAME_IMPL(_logMDScope) (key,\
    logging::detail::toMetadata(value));

#define ALOG_SCOPED_INDENT_IF_IMPL(channel, level) \
  logging::detail::CLogScopedIndent ALOG_UNIQUE_VAR_NAME_IMPL(__alog_scoped_indent__)(\
    channel, logging::detail::ELogLevels::  level)

#define _ALOG_FUNCTION __FUNCTION__

#define ALOG_FUNCTION_IMPL(channel, level, msg)\
  ALOG_SCOPED_BLOCK_IMPL(channel, level, "" << _ALOG_FUNCTION << "( " << msg << " )");\
  ALOG_SCOPED_INDENT_IF_IMPL(channel, level)

#define ALOG_IS_ENABLED_IMPL(channel, level)\
  logging::detail::CLogChannelRegistrySingleton::instance()->filter(\
    channel, logging::detail::ELogLevels:: level)

/*-- Setup Macros ------------------------------------------------------------*/

/* These macros are designed to be the only used interface to the logging
 * infrastructure. This allows compile-time removal of all logging for
 * performance by defining DISABLE_LOGGING */

 /** \brief Set up logging for an executable
  *
  * This setup macro should be called once per executable to configure logging
  * for the duration of execution. If re-configuration is needed, use
  * ALOG_RESET (such as in unit tests). This macro will configure logging to
  * log to a file if specified and/or to the screen if specified, and will set
  * the default level filter as well as channel specific filters.
  *
  * \param filename - The name for the log file. If empty, no file log is used
  * \param toScreen - If true, a stream log to std::cout is initialized
  * \param defaultLevel - The level to use by default when filtering log lines
  * \param filterSpec - A string specifying the filters to use for specific
  *   channels in the form "CH1:lvl1,CH2:lvl2"
  */
#ifndef DISABLE_LOGGING
#define ALOG_SETUP(filename, toScreen, defaultLevel, filterSpec)\
  std::shared_ptr<std::ofstream> __logFile;\
  logging::detail::CLogChannelRegistrySingleton\
    ::instance()->setupFilters(filterSpec, defaultLevel);\
  if (not std::string(filename).empty())\
    __logFile = logging::detail::InitLogFile(filename);\
  if (toScreen) logging::detail::InitLogStream(std::cout)
#else
#define ALOG_SETUP(filename, toScreen, defaultLevel, filterSpec)
#endif

#ifndef DISABLE_LOGGING
#define ALOG_ADJUST_LEVELS(defaultLevel, filterSpec)\
  logging::detail::CLogChannelRegistrySingleton\
    ::instance()->setupFilters(filterSpec, defaultLevel)
#else
#define ALOG_ADJUST_LEVELS(defaultLevel, filterSpec)
#endif

#ifndef DISABLE_LOGGING
#define ALOG_ENABLE_THREAD_ID()\
  logging::detail::CLogChannelRegistrySingleton::instance()->enableThreadID()
#else
#define ALOG_ENABLE_THREAD_ID()
#endif

#ifndef DISABLE_LOGGING
#define ALOG_DISABLE_THREAD_ID()\
  logging::detail::CLogChannelRegistrySingleton::instance()->disableThreadID()
#else
#define ALOG_DISABLE_THREAD_ID()
#endif

#ifndef DISABLE_LOGGING
#define ALOG_SERVICE_NAME(name)\
  logging::detail::CLogChannelRegistrySingleton::instance()->setServiceName(name)
#else
#define ALOG_SERVICE_NAME(name)
#endif

#ifndef DISABLE_LOGGING
#define ALOG_USE_STD_FORMATTER()\
  logging::detail::UseStdFormatter()
#else
#define ALOG_USE_STD_FORMATTER()
#endif

#ifndef DISABLE_LOGGING
#define ALOG_USE_JSON_FORMATTER()\
  logging::detail::UseJSONFormatter()
#else
#define ALOG_USE_JSON_FORMATTER()
#endif

#ifndef DISABLE_LOGGING
#define ALOG_RESET()\
  logging::detail::CLogChannelRegistrySingleton\
    ::instance()->reset()
#else
#define ALOG_RESET()
#endif


/** Define a member function that will be used with the XXXthis macros */
#ifndef DISABLE_LOGGING
#define ALOG_USE_CHANNEL(channel) \
  inline static std::string getLogChannel()\
  { static const std::string l_channel = #channel;\
    return l_channel; }
#else
#define ALOG_USE_CHANNEL(channel)
#endif

/** Define a free function that will be used with the XXXthis macros. This
 * should only be used in a main compilation unit */
#ifndef DISABLE_LOGGING
#define ALOG_USE_CHANNEL_FREE(channel) \
  inline std::string getLogChannel()\
  { static const std::string l_channel = #channel;\
    return l_channel; }
#else
#define ALOG_USE_CHANNEL_FREE(channel)
#endif

/*-- Log Macros --------------------------------------------------------------*/

/** Log a line on the given channel at the given level */
#ifndef DISABLE_LOGGING
#define ALOG(channel, level, msg) ALOG_CHANNEL_IMPL(#channel, level, msg)
#else
#define ALOG(channel, level, msg)
#endif

/** Log a line on the class' native channel at the given level */
#ifndef DISABLE_LOGGING
#define ALOGthis(level, msg) ALOG_CHANNEL_IMPL(getLogChannel(), level, msg)
#else
#define ALOGthis(level, msg)
#endif

/** Log an arbitrary key/value structure on the given channel/level */
#ifndef DISABLE_LOGGING
#define ALOG_MAP(channel, level, map) ALOG_MAP_IMPL(#channel, level, map)
#else
#define ALOG_MAP(channel, level, map)
#endif

/** Log an arbitrary key/value structure on the class' native channel */
#ifndef DISABLE_LOGGING
#define ALOG_MAPthis(level, map) ALOG_MAP_IMPL(getLogChannel(), level, map)
#else
#define ALOG_MAPthis(level, map)
#endif

/** Log a wchar line on the given channel at the given level */
#ifndef DISABLE_LOGGING
#define ALOGW(channel, level, msg) ALOGW_CHANNEL_IMPL(#channel, level, msg)
#else
#define ALOGW(channel, level, msg)
#endif

/** Log a wchar line on the class' native channel at the given level */
#ifndef DISABLE_LOGGING
#define ALOGWthis(level, msg) ALOGW_CHANNEL_IMPL(getLogChannel(), level, msg)
#else
#define ALOGWthis(level, msg)
#endif

/** Log a line that explicitly includes the thread id regardless of the global
 * setting */
#ifndef DISABLE_LOGGING
#define ALOG_THREAD(channel, level, msg)\
  ALOG(channel, level, "[" << std::this_thread::get_id() << "] " << msg)
#else
#define ALOG_THREAD(channel, level, msg)
#endif

/** Log a line that includes the current thread's thread id to the class'
 * native channel */
#ifndef DISABLE_LOGGING
#define ALOG_THREADthis(level, msg)\
  { auto& sngl = logging::detail::CLogChannelRegistrySingleton::instance();\
    bool currentlyEnabled = sngl->threadIDEnabled();\
    if (not currentlyEnabled) sngl->enableThreadID();\
    ALOGthis(level, msg);\
    if (not currentlyEnabled) sngl->disableThreadID(); }
#else
#define ALOG_THREADthis(level, msg)
#endif

/** Set up a Start/End block of logging based on the scope. Note that only a
 * single call to ALOG_SCOPED_BLOCK may be made within a given scope */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_BLOCK(channel, level, msg)\
  ALOG_SCOPED_BLOCK_IMPL(#channel, level, msg)
#else
#define ALOG_SCOPED_BLOCK(channel, level, msg)
#endif

/** Set up a Start/End block of logging based on the scope using class' native
 * channel */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_BLOCKthis(level, msg)\
  ALOG_SCOPED_BLOCK_IMPL(getLogChannel(), level, msg)
#else
#define ALOG_SCOPED_BLOCKthis(level, msg)
#endif

/** Set up a timer that will time the work done in the current scope and
 * report the duration upon scope completion */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_TIMER(channel, level, msg)\
  ALOG_SCOPED_TIMER_IMPL(#channel, level, msg)
#else
#define ALOG_SCOPED_TIMER(channel, level, msg)
#endif

/** Set up a timer that will time the work done in the current scope and
 * report the duration upon scope completion using current class' native
 * channel */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_TIMERthis(level, msg)\
  ALOG_SCOPED_TIMER_IMPL(getLogChannel(), level, msg)
#else
#define ALOG_SCOPED_TIMERthis(level, msg)
#endif

/** Set up a metadata scope that will add a key to the metadata that will be
 * removed when the current scope closes */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_METADATA(key, value)\
  ALOG_SCOPED_METADATA_IMPL(key, value)
#else
#define ALOG_SCOPED_METADATA(key, value)
#endif

/** Add a level of indentation to the current scope */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_INDENT() logging::detail::CLogScopedIndent \
  ALOG_UNIQUE_VAR_NAME_IMPL(__alog_scoped_indent__)
#else
#define ALOG_SCOPED_INDENT()
#endif

/** Add a level of indentation to the current scope if the given channel/level
 * is enabled */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_INDENT_IF(channel, level) ALOG_SCOPED_INDENT_IF_IMPL(#channel, level)
#else
#define ALOG_SCOPED_INDENT_IF(channel, level)
#endif

/** Add a level of indentation to the current scope if the given level is
 * enabled for the configured channel */
#ifndef DISABLE_LOGGING
#define ALOG_SCOPED_INDENT_IFthis(level) ALOG_SCOPED_INDENT_IF_IMPL(getLogChannel(), level)
#else
#define ALOG_SCOPED_INDENT_IFthis(level)
#endif

/** Add a Start/End indented block with the current function name on trace */
#ifndef DISABLE_LOGGING
#define ALOG_FUNCTION(channel, msg) ALOG_FUNCTION_IMPL(#channel, trace, msg)
#else
#define ALOG_FUNCTION(channel, msg)
#endif

/** Add a Start/End indented block with the current function name on trace
 * using native channel */
#ifndef DISABLE_LOGGING
#define ALOG_FUNCTIONthis(msg) ALOG_FUNCTION_IMPL(getLogChannel(), trace, msg)
#else
#define ALOG_FUNCTIONthis(msg)
#endif

/** Add a Start/End indented block with the current function name on designated
 * level. Used for lower-level functions that log on debug levels */
#ifndef DISABLE_LOGGING
#define ALOG_DETAIL_FUNCTION(channel, level, msg) ALOG_FUNCTION_IMPL(#channel, level, msg)
#else
#define ALOG_DETAIL_FUNCTION(channel, level, msg)
#endif

/** Add a Start/End indented block with the current function name on designated
 * level on the native channel. Used for lower-level functions that log on
 * debug levels */
#ifndef DISABLE_LOGGING
#define ALOG_DETAIL_FUNCTIONthis(level, msg) ALOG_FUNCTION_IMPL(getLogChannel(), level, msg)
#else
#define ALOG_DETAIL_FUNCTIONthis(level, msg)
#endif

/** This macro sends a warning to cerr and to the the log */
#define ALOG_WARNING(msg)\
  ALOG(WARN, warning, msg);\
  std::cerr << "WARNING: " << msg << std::endl

/** Resolve to a statement which is true if the given channel/level is enabled
 * and false otherwise */
#ifndef DISABLE_LOGGING
#define ALOG_IS_ENABLED(channel, level) ALOG_IS_ENABLED_IMPL(#channel, level)
#else
#define ALOG_IS_ENABLED(channel, level) false
#endif

/** Resolve to a statement which is true if the given level is enabled for the
 * native channel and false otherwise */
#ifndef DISABLE_LOGGING
#define ALOG_IS_ENABLEDthis(level) ALOG_IS_ENABLED_IMPL(getLogChannel(), level)
#else
#define ALOG_IS_ENABLEDthis(level) false
#endif
