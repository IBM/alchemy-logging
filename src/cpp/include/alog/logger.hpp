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
#include <iostream>

// Third Party
#include <nlohmann/json.hpp>

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
            nlohmann::json a_mapData = nlohmann::json{});
  std::string     channel;
  ELogLevels      level;
  std::string     message;
  std::string     timestamp;
  std::string     serviceName;
  unsigned        nIndent;
  std::thread::id threadId;
  nlohmann::json  mapData;
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

  /** Determine whether ID threading is currently enabled */
  bool threadIDEnabled() const { return m_doThreadLog; }

  /** Enable/disable metadata logging */
  void enableMetadata();
  void disableMetadata();

  /** Determine whether metadata is currently enabled */
  bool metadataEnabled() const { return m_doMetadata; }

  /** Set the service name to use */
  void setServiceName(const std::string&);
  const std::string& getServiceName() const { return m_serviceName; }

  /** Filter based on the channel and level. This is public so that it can be
   * run before pushing the message content to a stringstream in the macro */
  bool filter(const std::string& a_channel, ELogLevels) const;

  /** Send the given string to all sinks with proper formatting. Filtering is
   * done before this is called in ALOG, so this function does no filtering. */
  void log(const std::string& a_channel,
           ELogLevels a_level,
           const std::string& a_msg,
           nlohmann::json a_mapData);

  /** Send the given string to all sinks with proper formatting. Filtering is
   * done before this is called in ALOG, so this function does no filtering. */
  void log(const std::string& a_channel,
           ELogLevels a_level,
           const std::wstring& a_msg,
           nlohmann::json a_mapData);

  /** Add a level of indentation for the current thread */
  void addIndent();

  /** Remove a level of indentation for the current thread */
  void removeIndent();

  /** Get the indent level for the current thread */
  unsigned getIndent() const;

  /** Add a key to the metadata for the current thread */
  void addMetadata(const std::string& a_key, const nlohmann::json::value_type& a_value);

  /** Remove a key from the metadata for the current thread */
  void removeMetadata(const std::string& a_key);

  /** Clear the metadata for the current thread */
  void clearMetadata();

  /** Get a view into the current metadata dict for the current thread */
  const nlohmann::json& getMetadata() const;

  /** Clear the current filters and sinks and set the default level to off */
  void reset();

private:

  /* Private default constructor, deleted copy constructor and assignment
   * operator make this a singleton */
  CLogChannelRegistrySingleton()
    : m_defaultLevel(ELogLevels::off),
      m_doThreadLog(false),
      m_doMetadata(false)
  {};
  CLogChannelRegistrySingleton(const CLogChannelRegistrySingleton&) = delete;
  CLogChannelRegistrySingleton& operator=(const CLogChannelRegistrySingleton&) = delete;

  /* The global instance */
  static Ptr m_pInstance;

  FilterMap m_filters;
  ELogLevels m_defaultLevel;
  bool m_doThreadLog;
  bool m_doMetadata;
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

  typedef std::thread::id                     TThreadID;
  typedef std::map<TThreadID, unsigned>       ThreadIndentMap;
  typedef std::map<TThreadID, nlohmann::json> ThreadMetadataMap;
  ThreadIndentMap   m_indents;
  ThreadMetadataMap m_metadata;

};  // end class CLogChannelRegistrySingleton

/*-- Scope Classes -----------------------------------------------------------*/

typedef std::shared_ptr<const nlohmann::json> TScopeLogMapPtr;

/** \brief This class is used to add a Start/End block to a log */
class CLogScope
{
public:
  CLogScope(const CLogScope&) = delete;
  CLogScope(const std::string& a_channelName,
            ELogLevels a_level,
            const std::string& a_msg,
            const TScopeLogMapPtr& a_mapDataPtr = nullptr);
  virtual ~CLogScope();
private:
  const std::string     m_channelName;
  const ELogLevels      m_level;
  const std::string     m_msg;
  const TScopeLogMapPtr m_mapDataPtr;
};  // end class CLogScope

/** \brief Struct to time execution of a block */
struct CLogScopedTimer
{
  CLogScopedTimer(const std::string& a_channelName,
                  ELogLevels a_level,
                  const std::string& a_msg,
                  const TScopeLogMapPtr& a_mapDataPtr = nullptr);
  virtual ~CLogScopedTimer();

  /** \brief When created with ALOG_NEW_SCOPED_TIMER, this can be called to
   * get the current duration in nanoseconds */
  unsigned long getCurrentDurationNS() const;

private:
  const std::string     m_channelName;
  const ELogLevels      m_level;
  const std::string     m_msg;
  const TScopeLogMapPtr m_mapDataPtr;
  decltype(std::chrono::high_resolution_clock::now()) m_t0;
};  // end class CLogScopedTimer

/** \brief Struct to act as a proxy for logging indentation */
struct CLogScopedIndent
{
  CLogScopedIndent();
  CLogScopedIndent(const std::string& a_channelName,
                   ELogLevels a_level);
  ~CLogScopedIndent();

private:
  const bool m_enabled;
};  // end class CLogScopedIndent

/** \brief Struct to scope metadata entries */
struct CLogScopedMetadata
{
  CLogScopedMetadata(const std::string&, const nlohmann::json::value_type&);
  CLogScopedMetadata(const nlohmann::json&);
  ~CLogScopedMetadata();
private:
  const std::vector<std::string> m_keys;
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

} // end namespace detail

} // end namespace logging

/*-- Detail Macros -----------------------------------------------------------*/

// CITE: https://stackoverflow.com/questions/1082192/how-to-generate-random-variable-names-in-c-using-macros/1082211
// Note that this involves some black magic as noted by the above poster. It
// seems to work and since it's a macro, it only has to work at conpile time, so
// until such time that it stops compiling, we can trust it.
#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res
#define ALOG_UNIQUE_VAR_NAME_IMPL(base) PP_CAT(base, __LINE__)

// Some of the public macros can take an optional final map argument to add key/
// value data as needed. In order to enable this, we need the old NARGS trick:
// https://stackoverflow.com/questions/27765387/distributing-an-argument-in-a-variadic-macro
#define NARGS_MAP(...) NARGS_(__VA_ARGS__, _MAP, _NO_MAP, 0)
#define NARGS_NUM(...) NARGS_(__VA_ARGS__, _2, _1, 0)
#define NARGS_(_2, _1, N, ...) N
#define CONC(A, B) CONC_(A, B)
#define CONC_(A, B) A##B

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

// The channel functions can take an optional map argument
#define _ALOG_CHANNEL_IMPL_WITH_MAP(channel, level, msg, map) \
  ALOG_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, msg, map)
#define _ALOG_CHANNEL_IMPL_WITH_NO_MAP(channel, level, msg) \
  ALOG_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, msg, nlohmann::json{})

#define _ALOGW_CHANNEL_IMPL_WITH_MAP(channel, level, msg, map) \
  ALOGW_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, msg, map)
#define _ALOGW_CHANNEL_IMPL_WITH_NO_MAP(channel, level, msg) \
  ALOGW_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, msg, nlohmann::json{})

#define ALOG_CHANNEL_IMPL(channel, level, ...)\
  CONC(_ALOG_CHANNEL_IMPL_WITH, NARGS_MAP(__VA_ARGS__)) (channel, level, __VA_ARGS__)

#define ALOGW_CHANNEL_IMPL(channel, level, ...)\
  CONC(_ALOGW_CHANNEL_IMPL_WITH, NARGS_MAP(__VA_ARGS__)) (channel, level, __VA_ARGS__)

#define ALOG_MAP_IMPL(channel, level, map)\
  ALOG_LEVEL_IMPL(channel, logging::detail::ELogLevels:: level, "", map)

// All of the scope calls take a mapDataPtr as an optional last argument
#define _SCOPE_WRAPPER_WITH_MAP(scopeType, varName, channel, level, msg, map) \
  logging::detail:: scopeType varName (\
    channel, logging::detail::ELogLevels:: level,\
    static_cast<std::ostringstream&>(std::ostringstream().flush() << msg).str(), map)
#define _SCOPE_WRAPPER_WITH_NO_MAP(scopeType, varName, channel, level, msg) \
  logging::detail:: scopeType varName (\
    channel, logging::detail::ELogLevels:: level,\
    static_cast<std::ostringstream&>(std::ostringstream().flush() << msg).str())
#define _SCOPE_WRAPPER(scopeType, varName, channel, level, ...) \
  CONC(_SCOPE_WRAPPER_WITH, NARGS_MAP(__VA_ARGS__)) (scopeType, varName, channel, level, __VA_ARGS__)

#define ALOG_SCOPED_BLOCK_IMPL(channel, level, ...)\
  _SCOPE_WRAPPER(CLogScope, ALOG_UNIQUE_VAR_NAME_IMPL(CLogScope),\
    channel, level, __VA_ARGS__)

#define ALOG_SCOPED_TIMER_IMPL(channel, level, ...)\
  _SCOPE_WRAPPER(CLogScopedTimer, ALOG_UNIQUE_VAR_NAME_IMPL(CLogScopedTimer),\
    channel, level, __VA_ARGS__)

#define ALOG_NEW_SCOPED_TIMER_IMPL(channel, level, ...)\
  _SCOPE_WRAPPER(CLogScopedTimer, ,\
    channel, level, __VA_ARGS__)


#define _ALOG_FUNCTION __FUNCTION__

#define _ALOG_FUNCTION_IMPL_WITH_MAP(channel, level, msg, map)\
  ALOG_SCOPED_BLOCK_IMPL(channel, level, "" << _ALOG_FUNCTION << "( " << msg << " )", map);\
  ALOG_SCOPED_INDENT_IF_IMPL(channel, level)
#define _ALOG_FUNCTION_IMPL_WITH_NO_MAP(channel, level, msg)\
  ALOG_SCOPED_BLOCK_IMPL(channel, level, "" << _ALOG_FUNCTION << "( " << msg << " )");\
  ALOG_SCOPED_INDENT_IF_IMPL(channel, level)
#define ALOG_FUNCTION_IMPL(channel, level, ...)\
  CONC(_ALOG_FUNCTION_IMPL_WITH, NARGS_MAP(__VA_ARGS__)) (channel, level, __VA_ARGS__)

#define ALOG_IS_ENABLED_IMPL(channel, level)\
  logging::detail::CLogChannelRegistrySingleton::instance()->filter(\
    channel, logging::detail::ELogLevels:: level)

// Non-logging scope objects
#define _ALOG_SCOPED_METADATA_IMPL_2(key, value)\
  logging::detail::CLogScopedMetadata ALOG_UNIQUE_VAR_NAME_IMPL(_logMDScope) (key, value);
#define _ALOG_SCOPED_METADATA_IMPL_1(map)\
  logging::detail::CLogScopedMetadata ALOG_UNIQUE_VAR_NAME_IMPL(_logMDScope) (map);
#define ALOG_SCOPED_METADATA_IMPL(...)\
  CONC(_ALOG_SCOPED_METADATA_IMPL, NARGS_NUM(__VA_ARGS__)) (__VA_ARGS__)


#define ALOG_SCOPED_INDENT_IF_IMPL(channel, level) \
  logging::detail::CLogScopedIndent ALOG_UNIQUE_VAR_NAME_IMPL(__alog_scoped_indent__)(\
    channel, logging::detail::ELogLevels::  level)


/*-- PUBLIC INTERFACE --------------------------------------------------------*
 * These functions and macros are designed to be the only used interface to   *
 * the logging infrastructure.                                                *
 *----------------------------------------------------------------------------*/

/*-- Setup Functions ---------------------------------------------------------*/

/** \brief Set up logging for an executable
 *
 * This setup function should be called once per executable to configure logging
 * for the duration of execution. If re-configuration is needed, use
 * ALOG_RESET (such as in unit tests). This function will will set the default
 * level filter as well as channel specific filters and configure the output to
 * go to stdout.
 *
 * DEPRECATION NOTICE: The filename and toScreen arguments have been deprecated
 * and no longer function. The were not used, and were only here for legacy
 * reason.
 *
 * DEPRECATED \param filename - The name for the log file. If empty, no file
 *   log is used
 * DEPRECATED \param toScreen - If true, a stream log to std::cout is
 *   initialized
 * \param defaultLevel - The level to use by default when filtering log lines
 * \param filterSpec - A string specifying the filters to use for specific
 *   channels in the form "CH1:lvl1,CH2:lvl2"
 */
void ALOG_SETUP(
  const std::string& defaultLevel,
  const std::string& filterSpec);

/** \brief Adjust the global log levels
 *
 * \param defaultLevel - The level to use by default when filtering log lines
 * \param filterSpec - A string specifying the filters to use for specific
 *   channels in the form "CH1:lvl1,CH2:lvl2"
 */
void ALOG_ADJUST_LEVELS(
  const std::string& defaultLevel,
  const std::string& filterSpec);

/** \brief Enable logging the thread ID with each message */
void ALOG_ENABLE_THREAD_ID();

/** \brief Disable logging the thread ID with each message */
void ALOG_DISABLE_THREAD_ID();

/** \brief Enable logging of user-defined metadata with each message */
void ALOG_ENABLE_METADATA();

/** \brief Disable logging of user-defined metadata with each message */
void ALOG_DISABLE_METADATA();

/** \brief Set a service name to be logged with each message */
void ALOG_SERVICE_NAME(const std::string& name);

/** \brief Configure ALog to use the pretty-print formatter */
void ALOG_USE_STD_FORMATTER();

/** \brief Configure ALog to use the json formatter */
void ALOG_USE_JSON_FORMATTER();

/** \brief Reset ALog to the default (unconfigured) state */
void ALOG_RESET();


/** Define a member function that will be used with the XXXthis macros */
#define ALOG_USE_CHANNEL(channel) \
  inline static std::string getLogChannel()\
  { static const std::string l_channel = #channel;\
    return l_channel; }

/** Define a free function that will be used with the XXXthis macros. This
 * should only be used in a main compilation unit */
#define ALOG_USE_CHANNEL_FREE(channel) \
  inline std::string getLogChannel()\
  { static const std::string l_channel = #channel;\
    return l_channel; }

/*-- Log Macros --------------------------------------------------------------*/

/** Log a line on the given channel at the given level */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG(channel, level, ...) ALOG_CHANNEL_IMPL(#channel, level, __VA_ARGS__)
#else
#define ALOG(channel, level, ...)
#endif

/** Log a line on the class' native channel at the given level */
#ifndef ALOG_DISABLE_LOGGING
#define ALOGthis(level, ...) ALOG_CHANNEL_IMPL(getLogChannel(), level, __VA_ARGS__)
#else
#define ALOGthis(level, ...)
#endif

/** Log a wchar line on the given channel at the given level */
#ifndef ALOG_DISABLE_LOGGING
#define ALOGW(channel, level, ...) ALOGW_CHANNEL_IMPL(#channel, level, __VA_ARGS__)
#else
#define ALOGW(channel, level, ...)
#endif

/** Log a wchar line on the class' native channel at the given level */
#ifndef ALOG_DISABLE_LOGGING
#define ALOGWthis(level, ...) ALOGW_CHANNEL_IMPL(getLogChannel(), level, __VA_ARGS__)
#else
#define ALOGWthis(level, ...)
#endif

/** Log an arbitrary key/value structure on the given channel/level */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_MAP(channel, level, map) ALOG_MAP_IMPL(#channel, level, map)
#else
#define ALOG_MAP(channel, level, map)
#endif

/** Log an arbitrary key/value structure on the class' native channel */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_MAPthis(level, map) ALOG_MAP_IMPL(getLogChannel(), level, map)
#else
#define ALOG_MAPthis(level, map)
#endif

/** Log a line that explicitly includes the thread id regardless of the global
 * setting */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_THREAD(channel, level, ...)\
  { auto& sngl = logging::detail::CLogChannelRegistrySingleton::instance();\
    bool currentlyEnabled = sngl->threadIDEnabled();\
    if (not currentlyEnabled) sngl->enableThreadID();\
    ALOG(channel, level, __VA_ARGS__);\
    if (not currentlyEnabled) sngl->disableThreadID(); }
#else
#define ALOG_THREAD(channel, level, ...)
#endif

/** Log a line that includes the current thread's thread id to the class'
 * native channel */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_THREADthis(level, ...)\
  { auto& sngl = logging::detail::CLogChannelRegistrySingleton::instance();\
    bool currentlyEnabled = sngl->threadIDEnabled();\
    if (not currentlyEnabled) sngl->enableThreadID();\
    ALOGthis(level, __VA_ARGS__);\
    if (not currentlyEnabled) sngl->disableThreadID(); }
#else
#define ALOG_THREADthis(level, ...)
#endif

/** Set up a Start/End block of logging based on the scope. Note that only a
 * single call to ALOG_SCOPED_BLOCK may be made within a given scope */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_BLOCK(channel, level, ...)\
  ALOG_SCOPED_BLOCK_IMPL(#channel, level, __VA_ARGS__)
#else
#define ALOG_SCOPED_BLOCK(channel, level, ...)
#endif

/** Set up a Start/End block of logging based on the scope using class' native
 * channel */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_BLOCKthis(level, ...)\
  ALOG_SCOPED_BLOCK_IMPL(getLogChannel(), level, __VA_ARGS__)
#else
#define ALOG_SCOPED_BLOCKthis(level, ...)
#endif

/** Set up a timer that will time the work done in the current scope and
 * report the duration upon scope completion */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_TIMER(channel, level, ...)\
  ALOG_SCOPED_TIMER_IMPL(#channel, level, __VA_ARGS__)
#else
#define ALOG_SCOPED_TIMER(channel, level, ...)
#endif

/** Set up a timer that will time the work done in the current scope and
 * report the duration upon scope completion using current class' native
 * channel */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_TIMERthis(level, ...)\
  ALOG_SCOPED_TIMER_IMPL(getLogChannel(), level, __VA_ARGS__)
#else
#define ALOG_SCOPED_TIMERthis(level, ...)
#endif

/** Create a new instance of a scoped timer that can be queried for the current
 * duration and will report total time upon destruction
 *
 * NOTE: Since this creates a usable object, it can not be compiled out
 */
#define ALOG_NEW_SCOPED_TIMER(channel, level, ...)\
  ALOG_NEW_SCOPED_TIMER_IMPL(#channel, level, __VA_ARGS__)

/** Create a new instance of a scoped timer that can be queried for the current
 * duration and will report total time upon destruction using the current class'
 * native channel
 *
 * NOTE: Since this creates a usable object, it can not be compiled out
 */
#define ALOG_NEW_SCOPED_TIMERthis(level, ...)\
  ALOG_NEW_SCOPED_TIMER_IMPL(getLogChannel(), level, __VA_ARGS__)

/** Set up a metadata scope that will add a key to the metadata that will be
 * removed when the current scope closes */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_METADATA(...)\
  ALOG_SCOPED_METADATA_IMPL(__VA_ARGS__)
#else
#define ALOG_SCOPED_METADATA(...)
#endif

/** Add a level of indentation to the current scope */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_INDENT() logging::detail::CLogScopedIndent \
  ALOG_UNIQUE_VAR_NAME_IMPL(__alog_scoped_indent__)
#else
#define ALOG_SCOPED_INDENT()
#endif

/** Add a level of indentation to the current scope if the given channel/level
 * is enabled */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_INDENT_IF(channel, level) ALOG_SCOPED_INDENT_IF_IMPL(#channel, level)
#else
#define ALOG_SCOPED_INDENT_IF(channel, level)
#endif

/** Add a level of indentation to the current scope if the given level is
 * enabled for the configured channel */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_SCOPED_INDENT_IFthis(level) ALOG_SCOPED_INDENT_IF_IMPL(getLogChannel(), level)
#else
#define ALOG_SCOPED_INDENT_IFthis(level)
#endif

/** Add a Start/End indented block with the current function name on trace */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_FUNCTION(channel, ...) ALOG_FUNCTION_IMPL(#channel, trace, __VA_ARGS__)
#else
#define ALOG_FUNCTION(channel, ...)
#endif

/** Add a Start/End indented block with the current function name on trace
 * using native channel */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_FUNCTIONthis(...) ALOG_FUNCTION_IMPL(getLogChannel(), trace, __VA_ARGS__)
#else
#define ALOG_FUNCTIONthis(...)
#endif

/** Add a Start/End indented block with the current function name on designated
 * level. Used for lower-level functions that log on debug levels */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_DETAIL_FUNCTION(channel, level, ...) ALOG_FUNCTION_IMPL(#channel, level, __VA_ARGS__)
#else
#define ALOG_DETAIL_FUNCTION(channel, level, ...)
#endif

/** Add a Start/End indented block with the current function name on designated
 * level on the native channel. Used for lower-level functions that log on
 * debug levels */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_DETAIL_FUNCTIONthis(level, ...) ALOG_FUNCTION_IMPL(getLogChannel(), level, __VA_ARGS__)
#else
#define ALOG_DETAIL_FUNCTIONthis(level, ...)
#endif

/** This macro sends a warning to cerr and to the the log */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_WARNING(msg)\
  ALOG(WARN, warning, msg);\
  std::cerr << "WARNING: " << msg << std::endl
#else
#define ALOG_WARNING(msg)
#endif

/** Resolve to a statement which is true if the given channel/level is enabled
 * and false otherwise */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_IS_ENABLED(channel, level) ALOG_IS_ENABLED_IMPL(#channel, level)
#else
#define ALOG_IS_ENABLED(channel, level) false
#endif

/** Resolve to a statement which is true if the given level is enabled for the
 * native channel and false otherwise */
#ifndef ALOG_DISABLE_LOGGING
#define ALOG_IS_ENABLEDthis(level) ALOG_IS_ENABLED_IMPL(getLogChannel(), level)
#else
#define ALOG_IS_ENABLEDthis(level) false
#endif
