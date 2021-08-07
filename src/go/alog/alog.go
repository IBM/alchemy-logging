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

package alog

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"
)

//-- Public Types --------------------------------------------------------------

// LogLevel - Type used to represent hierarchical levels
type LogLevel int

// LogChannel - Type used to represent orthogonal logging channels
type LogChannel string

// ChannelMap - Type to use for the mapping from channel to level
type ChannelMap map[LogChannel]LogLevel

// Sequential log levels
const (
	OFF LogLevel = iota
	FATAL
	ERROR
	WARNING
	INFO
	TRACE
	DEBUG
	DEBUG1
	DEBUG2
	DEBUG3
	DEBUG4
)

// LogEntry - The individual entry struct containing all information needed to
// render the entry as a log line.
type LogEntry struct {
	Level       LogLevel
	Channel     LogChannel
	Format      string
	Expansion   []interface{}
	Timestamp   time.Time
	NIndent     int
	Servicename string
	GoroutineID *uint64
	MapData     map[string]interface{}
}

//-- Public Interfaces ---------------------------------------------------------

// LogFormatter - Interface for formatting and printing output from a LogEntry
type LogFormatter interface {
	FormatEntry(LogEntry) []string
}

// ScopedLogger - Interface for a scoped logger that logs Start/End blocks
type ScopedLogger interface {
	Close()
}

// ChannelLog - Interface for logger that always logs to a specific channel
type ChannelLog interface {
	Log(level LogLevel, format string, v ...interface{})
	Printf(level LogLevel, format string, v ...interface{})
	Panicf(level LogLevel, format string, v ...interface{})
	Fatalf(level LogLevel, format string, v ...interface{})
	LogMap(level LogLevel, mapData map[string]interface{})
	IsEnabled(level LogLevel) bool
	LogScope(level LogLevel, format string, v ...interface{}) ScopedLogger
	FnLog(format string, v ...interface{}) ScopedLogger
	DetailFnLog(level LogLevel, format string, v ...interface{}) ScopedLogger
}

//-- Core Implementation -------------------------------------------------------

// Core log config struct
type alogger struct {

	// Mutex to use for all access to the data in this struct
	mutex sync.RWMutex

	// The output writer
	writer io.Writer

	// Map from channel to level for specific channel configuration
	channelMap ChannelMap

	// Default level to use for channels that aren't specifically configured
	defaultLevel LogLevel

	// Length of the channel section of the header
	channelHeaderLen int

	// Optional service name string
	serviceName string

	// String to use for each individual indent
	indent string

	// Current indentation level per GID
	indentMap map[uint64]int

	// Bool to enable/disable indentation
	enableIndent bool

	// Bool to enable/disable displaying the goroutine ID in the header
	enableGID bool

	// Bool to enable/disable displaying the full function signature for FnLog
	fullFuncSig bool

	// The configured log formatter
	formatter LogFormatter
}

// This function converts a level to a 4-character header string that is used
// as part of the left-hand header for a log statement.
func levelToHeaderString(level LogLevel) string {
	switch level {
	case FATAL:
		return "FATL"
	case ERROR:
		return "ERRR"
	case WARNING:
		return "WARN"
	case INFO:
		return "INFO"
	case TRACE:
		return "TRCE"
	case DEBUG:
		return "DBUG"
	case DEBUG1:
		return "DBG1"
	case DEBUG2:
		return "DBG2"
	case DEBUG3:
		return "DBG3"
	case DEBUG4:
		return "DBG4"
	default:
		return "UNKN"
	}
}

func getGID() uint64 {
	b := make([]byte, 64)
	b = b[:runtime.Stack(b, false)]
	b = bytes.TrimPrefix(b, []byte("goroutine "))
	b = b[:bytes.IndexByte(b, ' ')]
	n, _ := strconv.ParseUint(string(b), 10, 64)
	return n
}

// The primary "enabled" check to preempt work when not needed
//
// NOTE: This does not provide a lock since it is an implementation only
//  function. Any use of it must be inside a read lock
////
func (cfg *alogger) isEnabled(channel LogChannel, level LogLevel) bool {
	chanLvl := cfg.defaultLevel
	if cLvl, ok := cfg.channelMap[channel]; ok {
		chanLvl = cLvl
	}
	return level > OFF && chanLvl >= level
}

// Implementation of the scoped logger that can't be created directly
type scopedLoggerImpl struct {
	channel LogChannel
	level   LogLevel
	format  string
	v       []interface{}
}

func (cfg *alogger) fnLogImpl(depth int, channel LogChannel, level LogLevel, format string, v ...interface{}) ScopedLogger {
	pc, _, _, _ := runtime.Caller(depth)
	name := runtime.FuncForPC(pc).Name()
	if !cfg.fullFuncSig {
		parts := strings.Split(name, ".")
		name = parts[len(parts)-1]
	}
	newFormat := fmt.Sprintf("%s(%s)", name, format)
	return LogScope(channel, level, newFormat, v...)
}

func (cfg *alogger) getIndentCount() int {
	nIndent := 0
	if cfg.enableIndent {
		gid := getGID()
		if n, ok := std.indentMap[gid]; ok {
			nIndent = n
		}
	}
	return nIndent
}

func (cfg *alogger) reset() {
	cfg.channelMap = ChannelMap{}
	cfg.defaultLevel = OFF
	cfg.channelHeaderLen = 5
	cfg.indent = "  "
	cfg.indentMap = map[uint64]int{}
	cfg.enableIndent = true
	cfg.enableGID = false
	cfg.fullFuncSig = false
	cfg.serviceName = ""
	cfg.formatter = StdLogFormatter{}
	cfg.writer = os.Stderr
}

func (cfg *alogger) formatTimestamp(ts time.Time) string {
	return fmt.Sprintf("%d/%02d/%02d %02d:%02d:%02d",
		ts.Year(), ts.Month(), ts.Day(), ts.Hour(), ts.Minute(), ts.Second())
}

func new() *alogger {
	l := &alogger{}
	l.reset()
	return l
}

// The package-level log instance
var std = new()

//-- StdLogFormatter Implementation --------------------------------------------

// StdLogFormatter - LogFormatter instance that wraps golang's log package
type StdLogFormatter struct{}

// Generate the header
func (p StdLogFormatter) makeHeader(e LogEntry) string {

	// Format the timestamp
	tsStr := std.formatTimestamp(e.Timestamp)

	// Format the serviceName if present
	svcNmStr := ""
	if len(e.Servicename) > 0 {
		svcNmStr = fmt.Sprintf(" <%s>", e.Servicename)
	}

	// Get the channel string
	chStr := e.Channel
	if len(e.Channel) > std.channelHeaderLen {
		chStr = e.Channel[:std.channelHeaderLen]
	} else if len(e.Channel) < std.channelHeaderLen {
		formatString := fmt.Sprintf("%%-%ds", std.channelHeaderLen)
		chStr = LogChannel(fmt.Sprintf(formatString, e.Channel))
	}

	// Get goroutine ID string
	gidString := ""
	gid := getGID()
	if std.enableGID {
		gidString = fmt.Sprintf(":%d", gid)
	}

	// Get the indent string
	indentStr := ""
	for i := 0; i < e.NIndent; i++ {
		indentStr = indentStr + std.indent
	}

	// Create the header
	return fmt.Sprintf("%s%s [%s:%s%s] %s", tsStr, svcNmStr, chStr, levelToHeaderString(e.Level), gidString, indentStr)
}

// FormatEntry - Format an entry using go's log package
func (p StdLogFormatter) FormatEntry(e LogEntry) []string {
	header := p.makeHeader(e)
	body := fmt.Sprintf(e.Format, e.Expansion...)
	out := []string{}
	if len(body) > 0 {
		for _, line := range strings.Split(body, "\n") {
			out = append(out, header+line+"\n")
		}
	}
	if len(e.MapData) > 0 {
		keys := []string{}
		for k := range e.MapData {
			keys = append(keys, k)
		}
		sort.Strings(keys)
		for _, k := range keys {
			out = append(out, header+fmt.Sprintf("%s: %v\n", k, e.MapData[k]))
		}
	}
	return out
}

//-- JSONLogFormatter Implementation ---------------------------------------------

// JSONLogFormatter - LogFormatter intance that prints LogEntry objects as json
type JSONLogFormatter struct{}

// FormatEntry - Implementation of the creation of the log string
func (p JSONLogFormatter) FormatEntry(e LogEntry) []string {

	// Set up the output json struct
	outMap := map[string]interface{}{}

	// Merge in map data
	for k, v := range e.MapData {
		outMap[k] = v
	}

	// Add standard fields
	outMap["channel"] = string(e.Channel)
	outMap["level_str"] = LevelToHumanString(e.Level)
	outMap["message"] = fmt.Sprintf(e.Format, e.Expansion...)
	outMap["timestamp"] = std.formatTimestamp(e.Timestamp)
	outMap["num_indent"] = e.NIndent
	outMap["service_name"] = e.Servicename

	// Add gid if enabled
	if std.enableGID {
		outMap["thread_id"] = getGID()
	}

	// Serialize to json
	out := []byte{}
	if jBytes, err := json.Marshal(outMap); nil != err {
		out = []byte(fmt.Sprintf("{\"error\": \"Failed to marshal json line [%v]\"}", err))
	} else {
		out = append(jBytes, '\n')
	}
	return []string{string(out)}
}

//-- Public Config Methods -----------------------------------------------------

// SetFormatter - Set the LogFormatter instance to use
func SetFormatter(f LogFormatter) {
	std.mutex.Lock()
	std.formatter = f
	std.mutex.Unlock()
}

// ResetDefaults - Reset to package default configuration
func ResetDefaults() {
	std.mutex.Lock()
	std.reset()
	std.mutex.Unlock()
}

// ConfigChannel - Set the level for a specific channel
func ConfigChannel(channel LogChannel, level LogLevel) {
	std.mutex.Lock()
	if nil == std.channelMap {
		std.channelMap = ChannelMap{}
	}
	std.channelMap[channel] = level
	std.mutex.Unlock()
}

// ConfigDefaultLevel - Set the level to use for channels not otherwise set
func ConfigDefaultLevel(level LogLevel) {
	std.mutex.Lock()
	std.defaultLevel = level
	std.mutex.Unlock()
}

// EnableIndent - Enable indentation tracking
func EnableIndent() {
	std.mutex.Lock()
	std.enableIndent = true
	std.mutex.Unlock()
}

// DisableIndent - Disable indentation tracking
func DisableIndent() {
	std.mutex.Lock()
	std.enableIndent = false
	std.mutex.Unlock()
}

// EnableGID - Enable logging the goroutine-id for each message
func EnableGID() {
	std.mutex.Lock()
	std.enableGID = true
	std.mutex.Unlock()
}

// DisableGID - Disable logging the goroutine-id for each message
func DisableGID() {
	std.mutex.Lock()
	std.enableGID = false
	std.mutex.Unlock()
}

// EnableFullFuncSig - Enable logging fully qualified function signatures
func EnableFullFuncSig() {
	std.mutex.Lock()
	std.fullFuncSig = true
	std.mutex.Unlock()
}

// DisableFullFuncSig - Disable logging fully qualified funciton signatures
func DisableFullFuncSig() {
	std.mutex.Lock()
	std.fullFuncSig = false
	std.mutex.Unlock()
}

// Config - Set the default level and channel filter map
func Config(defaultLevel LogLevel, channelMap ChannelMap) {
	std.mutex.Lock()
	std.defaultLevel = defaultLevel
	std.channelMap = channelMap
	std.mutex.Unlock()
}

// SetMaxChannelLen - Set the truncation length for channel headers
func SetMaxChannelLen(n int) {
	std.mutex.Lock()
	std.channelHeaderLen = n
	std.mutex.Unlock()
}

// UseJSONLogFormatter - Set the formatter to print JSON output lines
func UseJSONLogFormatter() {
	std.mutex.Lock()
	std.formatter = JSONLogFormatter{}
	std.mutex.Unlock()
}

// UseStdLogFormatter - Set the formatter to use the default StdLogFormatter
func UseStdLogFormatter() {
	std.mutex.Lock()
	std.formatter = StdLogFormatter{}
	std.mutex.Unlock()
}

// SetWriter - Set the io.Writer object to use
func SetWriter(w io.Writer) {
	std.mutex.Lock()
	std.writer = w
	std.mutex.Unlock()
}

// SetServiceName - Set a service name to be logged
func SetServiceName(sn string) {
	std.mutex.Lock()
	std.serviceName = sn
	std.mutex.Unlock()
}

//-- Public Log Methods --------------------------------------------------------

// Log - Alias to Printf. This is the standard log function.
func Log(channel LogChannel, level LogLevel, format string, v ...interface{}) {
	Printf(channel, level, format, v...)
}

// Printf - The standard Printf function. This wraps log.Printf
func Printf(channel LogChannel, level LogLevel, format string, v ...interface{}) {
	std.mutex.RLock()
	if std.isEnabled(channel, level) {
		for _, m := range std.formatter.FormatEntry(LogEntry{
			Channel:     channel,
			Level:       level,
			Format:      format,
			Expansion:   v,
			NIndent:     std.getIndentCount(),
			Timestamp:   time.Now().UTC(),
			Servicename: std.serviceName,
		}) {
			std.writer.Write([]byte(m))
		}
	}
	std.mutex.RUnlock()
}

// Fatalf - The standard Fatalf function. This wraps log.Fatalf
func Fatalf(channel LogChannel, level LogLevel, format string, v ...interface{}) {
	Printf(channel, level, format, v...)
	os.Exit(1)
}

// Panicf - The standard Panicf function. This wraps log.Panicf
func Panicf(channel LogChannel, level LogLevel, format string, v ...interface{}) {
	msg := ""
	std.mutex.RLock()
	if std.isEnabled(channel, level) {
		msg = strings.Join(std.formatter.FormatEntry(LogEntry{
			Channel:     channel,
			Level:       level,
			Format:      format,
			Expansion:   v,
			NIndent:     std.getIndentCount(),
			Timestamp:   time.Now().UTC(),
			Servicename: std.serviceName,
		}), "\n")
	}
	std.mutex.RUnlock()
	panic(msg)
}

// LogMap - Log a structured map entry
func LogMap(channel LogChannel, level LogLevel, mapData map[string]interface{}) {
	std.mutex.RLock()
	if std.isEnabled(channel, level) {
		for _, m := range std.formatter.FormatEntry(LogEntry{
			Channel:     channel,
			Level:       level,
			MapData:     mapData,
			NIndent:     std.getIndentCount(),
			Timestamp:   time.Now().UTC(),
			Servicename: std.serviceName,
		}) {
			std.writer.Write([]byte(m))
		}
	}
	std.mutex.RUnlock()
}

// LogWithMap - Log a message with additional structured map data
func LogWithMap(channel LogChannel, level LogLevel, mapData map[string]interface{}, format string, v ...interface{}) {
	std.mutex.RLock()
	if std.isEnabled(channel, level) {
		for _, m := range std.formatter.FormatEntry(LogEntry{
			Channel:     channel,
			Level:       level,
			Format:      format,
			Expansion:   v,
			MapData:     mapData,
			NIndent:     std.getIndentCount(),
			Timestamp:   time.Now().UTC(),
			Servicename: std.serviceName,
		}) {
			std.writer.Write([]byte(m))
		}
	}
	std.mutex.RUnlock()
}

//-- Convenience Methods -------------------------------------------------------

// Indent - Increase the indent level
func Indent() {
	std.mutex.Lock()
	if std.enableIndent {
		gid := getGID()
		nIndent := 0
		if n, ok := std.indentMap[gid]; ok {
			nIndent = n
		}
		nIndent++
		std.indentMap[gid] = nIndent
	}
	std.mutex.Unlock()
}

// Deindent - Decrease the indent level
func Deindent() {
	std.mutex.Lock()
	if std.enableIndent {
		gid := getGID()
		if n, ok := std.indentMap[gid]; ok {
			if n > 0 {
				std.indentMap[gid] = n - 1
			} else {
				delete(std.indentMap, gid)
			}
		}
	}
	std.mutex.Unlock()
}

// IsEnabled - Determine if a given channel/level combo is enabled
//
// NOTE: Using this can be dangerous if your program contains functionality
//  inside an if block using IsEnabled.
////
func IsEnabled(channel LogChannel, level LogLevel) bool {
	std.mutex.RLock()
	out := std.isEnabled(channel, level)
	std.mutex.RUnlock()
	return out
}

// Close - Closer for the scopedLoggerImpl type
//
// This is meant to be called in a defer statement in order to facilitate Start/
// End log statements. Typically this would look like:
//
// func foo() {
//   defer ch.LogScope(alog.INFO, "My message").Close()
// }
//
// You can also emulate a "local scope" (i.e. construct/destruct in curly
// braces) with an anonymous function:
//
// func foo() {
//   ch.Log(alog.INFO, "Hello there!")
//   func() {
//     defer ch.LogScope(alog.INFO, "My local scope").Close()
//     bar := 1
//     bat := 2
//     ch.Log(alog.INFO, "The bar is %d", bar)
//     ch.Log(alog.INFO, "The bat is %d", bat)
//   }
//   ch.Log(alog.INFO, "Log after the local scope is closed")
// }
////
func (scope *scopedLoggerImpl) Close() {
	Deindent()
	Log(scope.channel, scope.level, "End: "+scope.format, scope.v...)
}

// LogScope - Create a log scope object to log a Start/End block
func LogScope(channel LogChannel, level LogLevel, format string, v ...interface{}) ScopedLogger {
	Log(channel, level, "Start: "+format, v...)
	Indent()
	return &scopedLoggerImpl{
		channel: channel,
		level:   level,
		format:  format,
		v:       v,
	}
}

// FnLog - Create a log scope object with Start/End block containing the
// function signature. This is always logged to the TRACE level.
func FnLog(channel LogChannel, format string, v ...interface{}) ScopedLogger {
	return std.fnLogImpl(2, channel, TRACE, format, v...)
}

// DetailFnLog - Create a log scope object with Start/End block containing the
// function signature. This allows you to specify the log level.
func DetailFnLog(channel LogChannel, level LogLevel, format string, v ...interface{}) ScopedLogger {
	return std.fnLogImpl(2, channel, level, format, v...)
}

//-- Getters -------------------------------------------------------------------

// GetDefaultLevel - Get the configured default level
func GetDefaultLevel() LogLevel {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.defaultLevel
}

// GetChannelMap - Get the configured channel map
func GetChannelMap() ChannelMap {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.channelMap
}

// GetChannelHeaderLen - Get the configured channel header length
func GetChannelHeaderLen() int {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.channelHeaderLen
}

// GetServiceName - Get the configured service name
func GetServiceName() string {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.serviceName
}

// GetIndentString - Get a copy of the indent string
func GetIndentString() string {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.indent
}

// IndentEnabled - Get state of whether indentation is enabled
func IndentEnabled() bool {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.enableIndent
}

// GIDEnabled - Get state of whether the Goroutine ID is enabled
func GIDEnabled() bool {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.enableGID
}

// FuncSigEnabled - Get state of whether the full function signature is enabled
func FuncSigEnabled() bool {
	std.mutex.RLock()
	defer std.mutex.RUnlock()
	return std.fullFuncSig
}

// LevelToHumanString - Convert a level value to a human readable string
func LevelToHumanString(level LogLevel) string {
	switch level {
	case FATAL:
		return "fatal"
	case ERROR:
		return "error"
	case WARNING:
		return "warning"
	case INFO:
		return "info"
	case TRACE:
		return "trace"
	case DEBUG:
		return "debug"
	case DEBUG1:
		return "debug1"
	case DEBUG2:
		return "debug2"
	case DEBUG3:
		return "debug3"
	case DEBUG4:
		return "debug4"
	default:
		return "UNKNOWN"
	}
}

// PrintConfig - Create a string representation of the current configuration
func PrintConfig() string {
	s := fmt.Sprintf("Default Level: %s\n", levelToHeaderString(std.defaultLevel))
	s += fmt.Sprintf("Channel Map:\n")
	for k, v := range std.channelMap {
		s += fmt.Sprintf("  %s: %s\n", string(k), LevelToHumanString(v))
	}
	return s[:len(s)-1]
}

//-- Channel Log ---------------------------------------------------------------

// Implementation of the ChannelLog interface that can't be constructed directly
type channelLogImpl struct {
	channel LogChannel
}

// UseChannel - Create a channel object that allows subsequent log statements to
// use a pre-configured channel.
//
// The general usage of the UseChannel function is to set up a consistent
// channel name for logically grouped functionality. For example, a struct used
// to perform DOIT logic might have a member channel object that allows all
// functions on it to log to the "DOIT" channel:
//
// type Doit struct {
//   ch ChannelLog,
// }
//
// func (d *Doit) GetItDone() {
//   defer d.ch.FnLog("").Close()
//   d.ch.Log(alog.INFO, "It's DONE!")
// }
func UseChannel(channel LogChannel) ChannelLog {
	return &channelLogImpl{
		channel: channel,
	}
}

// Log - Log to a LogChannel instance
func (ch *channelLogImpl) Log(level LogLevel, format string, v ...interface{}) {
	Log(ch.channel, level, format, v...)
}

// Printf - Printf to a LogChannel instance
func (ch *channelLogImpl) Printf(level LogLevel, format string, v ...interface{}) {
	Printf(ch.channel, level, format, v...)
}

// Panicf - Panicf to a LogChannel instance
func (ch *channelLogImpl) Panicf(level LogLevel, format string, v ...interface{}) {
	Panicf(ch.channel, level, format, v...)
}

// Fatalf - Fatalf to a LogChannel instance
func (ch *channelLogImpl) Fatalf(level LogLevel, format string, v ...interface{}) {
	Fatalf(ch.channel, level, format, v...)
}

// LogMap - LogMap to a LogChannel instance
func (ch *channelLogImpl) LogMap(level LogLevel, mapData map[string]interface{}) {
	LogMap(ch.channel, level, mapData)
}

// LogWithMap - LogWithMap to a LogChannel instance
func (ch *channelLogImpl) LogWithMap(level LogLevel, mapData map[string]interface{}, format string, v ...interface{}) {
	LogWithMap(ch.channel, level, mapData, format, v...)
}

// IsEnabled - IsEnabled for a LogChannel instance
func (ch *channelLogImpl) IsEnabled(level LogLevel) bool {
	return IsEnabled(ch.channel, level)
}

// LogScope - LogScope for a LogChannel instance
func (ch *channelLogImpl) LogScope(level LogLevel, format string, v ...interface{}) ScopedLogger {
	return LogScope(ch.channel, level, format, v...)
}

// FnLog - FnLog for a LogChannel instance
func (ch *channelLogImpl) FnLog(format string, v ...interface{}) ScopedLogger {
	return std.fnLogImpl(2, ch.channel, TRACE, format, v...)
}

// DetailFnLog - DetailFnLog for a LogChannel instance
func (ch *channelLogImpl) DetailFnLog(level LogLevel, format string, v ...interface{}) ScopedLogger {
	return std.fnLogImpl(2, ch.channel, level, format, v...)
}
