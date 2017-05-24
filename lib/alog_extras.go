// *************************************************
// 5737-C06
// (C) Copyright IBM Corp. 2017 All Rights Reserved.
// The source code for this program is not published or otherwise
// divested of its trade secrets, irrespective of what has been
// deposited with the U.S. Copyright Office.
// *************************************************

package alog

import (
	"bytes"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"net/http"
	"reflect"
	"strconv"
	"strings"
	"sync"
	"time"
)

//-- General Helpers -----------------------------------------------------------

// LevelFromString - Parse an alog LogLevel from a string representation
func LevelFromString(s string) (LogLevel, error) {
	switch s {
	case "off":
		return OFF, nil
	case "fatal":
		return FATAL, nil
	case "error":
		return ERROR, nil
	case "warning":
		return WARNING, nil
	case "info":
		return INFO, nil
	case "trace":
		return TRACE, nil
	case "debug":
		return DEBUG, nil
	case "debug1":
		return DEBUG1, nil
	case "debug2":
		return DEBUG2, nil
	case "debug3":
		return DEBUG3, nil
	case "debug4":
		return DEBUG4, nil
	default:
		msg := fmt.Sprintf("Invalid log level [%s]", s)
		Log("MAIN", WARNING, msg)
		return ERROR, errors.New(msg)
	}
}

// ParseChannelFilter - Parse a per-channel filter map from a string
func ParseChannelFilter(s string) (ChannelMap, error) {
	cmap := ChannelMap{}
	var errOut error
	for _, entry := range strings.Split(s, ",") {
		if len(entry) > 0 {
			parts := strings.Split(entry, ":")
			if len(parts) != 2 {
				errOut = fmt.Errorf("Bad channel config found [%s]", entry)
				Log("MAIN", ERROR, errOut.Error())
			} else {
				k := LogChannel(string(parts[0]))
				if v, err := LevelFromString(string(parts[1])); nil != err {
					errOut = fmt.Errorf("Bad level specified: %s", parts[1])
					Log("MAIN", ERROR, errOut.Error())
				} else {
					cmap[k] = v
				}
			}
		}
	}
	if nil != errOut {
		return ChannelMap{}, errOut
	}
	return cmap, nil
}

//-- Command Line Helpers ------------------------------------------------------

// FlagSet - The set of flag variables to configure from the command line
type FlagSet struct {
	DefaultLevel     *string
	ChannelConfig    *string
	ChannelHeaderLen *int
	EnableGID        *bool
	EnableFuncSig    *bool
	DisableIndent    *bool
	ServiceName      *string
	OutputJSON       *bool
}

// GetFlags - Get the configured set of command line flags for alog
func GetFlags() FlagSet {
	return FlagSet{
		DefaultLevel: flag.String(
			"log.default-level",
			"info",
			"Default log level"),

		ChannelConfig: flag.String(
			"log.filters",
			"",
			"Per-channel log level configuration"),

		ChannelHeaderLen: flag.Int(
			"log.chan-header-len",
			5,
			"Maximum length for log channel strings in the header"),

		EnableGID: flag.Bool(
			"log.goroutine-id",
			false,
			"Log the numerica ID of the goroutine in the header"),

		EnableFuncSig: flag.Bool(
			"log.function-signature",
			false,
			"Log the full function signature for trace logging"),

		DisableIndent: flag.Bool(
			"log.no-indent",
			false,
			"Disable indentation"),

		ServiceName: flag.String(
			"log.service-name",
			"",
			"Set a service name to display with each log line"),

		OutputJSON: flag.Bool(
			"log.output-json",
			false,
			"Output log lines as structured JSON rather than plain text"),
	}
}

// ConfigureFromFlags - Configure the global alog setup from a FlagSet
func ConfigureFromFlags(aFlags FlagSet) error {
	ResetDefaults()
	var errOut error

	// Parse default level
	dfltLvl := ERROR
	if dl, err := LevelFromString(*(aFlags.DefaultLevel)); nil != err {
		errOut = errors.New("Invalid default level. Setting to ERROR")
		Log("MAIN", WARNING, errOut.Error())
	} else {
		dfltLvl = dl
	}

	// Parse channel filters
	cmap := ChannelMap{}
	if cm, err := ParseChannelFilter(*(aFlags.ChannelConfig)); nil != err {
		errOut = fmt.Errorf("Unable to parse channel map: %v", err)
	} else {
		cmap = cm
	}

	// Short-circuit if error parsing
	if nil != errOut {
		return errOut
	}

	// Configure level and channels
	Config(dfltLvl, cmap)

	// Max channel length
	SetMaxChannelLen(*(aFlags.ChannelHeaderLen))
	if *(aFlags.EnableGID) {
		EnableGID()
	} else {
		DisableGID()
	}

	// Full function signature
	if *(aFlags.EnableFuncSig) {
		EnableFullFuncSig()
	} else {
		DisableFullFuncSig()
	}

	// Indentation
	if *(aFlags.DisableIndent) {
		DisableIndent()
	} else {
		EnableIndent()
	}

	// Service Name
	if len(*(aFlags.ServiceName)) > 0 {
		SetServiceName(*aFlags.ServiceName)
	}

	// JSON output
	if *(aFlags.OutputJSON) {
		UseJSONLogFormatter()
	} else {
		UseStdLogFormatter()
	}

	Log("MAIN", INFO, "Logging Configured!")
	return errOut
}

//-- Dynamic Server Logging ----------------------------------------------------

// Struct to act as the global singleton for managing simultaneous dynamic logs
type dynamicLogLock struct {
	mutex       sync.Mutex
	timerActive bool
}

// Global singleton instance of the dynamicLogLock
var stdDynamicLogLock = &dynamicLogLock{}

// DynamicLogConfig - Configuration object for dynamic logging
type DynamicLogConfig struct {
	DefaultLevel string
	Filters      string
	Timeout      uint32
}

// ConfigureDynamicLogging - Set up global logging for runtime-dynamic logging
//
// NOTE: Errors from this function may be the result of bad user input, or may
//  be caused by attempting to call it when another temporary configuration is
//  active. To determine the type of the error, look for the string 'USER:' at
//  the beginning of the log message.
func ConfigureDynamicLogging(c DynamicLogConfig) error {
	ch := UseChannel("DYLOG")
	defer ch.FnLog("").Close()

	// Get the dynamic log lock and defer its release
	stdDynamicLogLock.mutex.Lock()
	defer stdDynamicLogLock.mutex.Unlock()

	// If a timer is currently active, we can't reconfigure right now
	if stdDynamicLogLock.timerActive {
		return errors.New("Cannot perform multiple temporary dynamic logs at once")
	}

	// Parse params
	level := GetDefaultLevel()
	cMap := ChannelMap{}
	var timeout *time.Duration
	{
		if len(c.DefaultLevel) > 0 {
			lvl, err := LevelFromString(c.DefaultLevel)
			if nil != err {
				errOut := fmt.Errorf("USER: Invalid default level specified: %s", c.DefaultLevel)
				ch.Log(WARNING, errOut.Error())
				return errOut
			}
			level = lvl
		}
		if len(c.Filters) > 0 {
			cm, err := ParseChannelFilter(c.Filters)
			if nil != err {
				errOut := fmt.Errorf("USER: Failed to parse channel map: %v", err)
				ch.Log(WARNING, errOut.Error())
				return errOut
			}
			for chnl, lvl := range cm {
				cMap[chnl] = lvl
			}
		}
		if c.Timeout > 0 {
			secs := time.Duration(c.Timeout) * time.Second
			timeout = &secs
		}
	}

	// Make the adjustment
	currentLevel := GetDefaultLevel()
	currentCMap := GetChannelMap()
	ch.Log(INFO, "Before adjustment:\n%s", PrintConfig())
	Config(level, cMap)
	ch.Log(INFO, "After adjustment:\n%s", PrintConfig())

	// If timeout given, set up the timeout function
	if nil != timeout {
		ch.Log(INFO, "Setting up adjustment to time out in %v", *timeout)
		stdDynamicLogLock.timerActive = true
		go func(lvl LogLevel, cm ChannelMap, dt time.Duration) {
			// Sleep for the desired amount of time
			time.Sleep(dt)

			// Reconfigure back to previous configuration
			ch.Log(INFO, "Resetting logging after timed adjust")
			ch.Log(INFO, "Before adjustment:\n%s", PrintConfig())
			Config(lvl, cm)
			ch.Log(INFO, "After adjustment:\n%s", PrintConfig())

			// Unblock future requests
			stdDynamicLogLock.mutex.Lock()
			defer stdDynamicLogLock.mutex.Unlock()
			stdDynamicLogLock.timerActive = false

		}(currentLevel, currentCMap, *timeout)
	}

	return nil
}

// DynamicHandler - Http handler instance that can modify the alog configuration
// at runtime.
//
// This handler supports the following query params:
//
// * default_level=xxx - Set the default log level
// * filters=AAA:bbb,CCC:ddd - Set the per-channel log level filters
// * timeout=X - Set a time at which the dynamic configuration should revert to
//    the current configuration
////
func DynamicHandler(w http.ResponseWriter, r *http.Request) {
	ch := UseChannel("DYLOG")
	defer ch.FnLog("").Close()

	// Parse params
	r.ParseForm()

	// Parse params
	config := DynamicLogConfig{}
	{
		r.ParseForm()
		for param, vals := range r.Form {
			if len(vals) > 0 {
				switch param {
				case "default_level":
					config.DefaultLevel = vals[len(vals)-1]
				case "filters":
					config.Filters = vals[len(vals)-1]
				case "timeout":
					if t, err := strconv.ParseUint(vals[len(vals)-1], 10, 32); nil == err {
						config.Timeout = uint32(t)
					}
				}
			}
		}
	}

	// Do the dynamic configuration
	if err := ConfigureDynamicLogging(config); nil != err {
		ch.Log(DEBUG, "Got error while trying to configure dynamic loging: %v", err)
		w.WriteHeader(http.StatusConflict)
	} else {
		w.WriteHeader(http.StatusOK)
	}
	return
}

//-- JSON to plain text --------------------------------------------------------

// JSONToLogEntry - Convert a structured JSON log line to its corresponding
// LogEntry object
func JSONToLogEntry(jsString string) (*LogEntry, error) {

	// Unmarshal to a generic map, using the Number type to decode numbers
	entryMap := map[string]interface{}{}
	decoder := json.NewDecoder(bytes.NewBuffer([]byte(jsString)))
	decoder.UseNumber()
	if err := decoder.Decode(&entryMap); nil != err {
		return nil, err
	}

	// Check required entries
	for _, k := range []string{"channel", "level_str", "message", "timestamp", "num_indent"} {
		if _, ok := entryMap[k]; !ok {
			return nil, fmt.Errorf("Missing required field '%s'", k)
		}
	}

	// Create a log entry and fill it
	le := LogEntry{}
	var outErr error
	for k, v := range entryMap {

		switch k {
		case "channel":

			// channel
			if strVal, ok := v.(string); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
			} else {
				le.Channel = LogChannel(strVal)
			}
		case "level_str":

			// level
			if strVal, ok := v.(string); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
			} else if lvl, err := LevelFromString(strVal); nil != err {
				outErr = fmt.Errorf("Bad level found: %s", strVal)
			} else {
				le.Level = lvl
			}
		case "message":

			// message
			if strVal, ok := v.(string); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
			} else {
				le.Format = strVal
			}
		case "timestamp":

			// timestamp
			if strVal, ok := v.(string); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
				outErr = fmt.Errorf("Bad type for '%s'", k)
			} else if ts, err := time.Parse("2006/01/02 15:04:05", strVal); nil != err {
			} else {
				le.Timestamp = ts
			}
		case "num_indent":

			// num_indent
			if numVal, ok := v.(json.Number); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
			} else if intVal, err := numVal.Int64(); nil != err {
				outErr = fmt.Errorf("Wrong number type for '%s' - %s", k, numVal.String())
			} else {
				le.NIndent = int(intVal)
			}
		case "service_name":

			// service_name
			if strVal, ok := v.(string); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
			} else {
				le.Servicename = strVal
			}
		case "thread_id":

			// thread_id
			if numVal, ok := v.(json.Number); !ok {
				outErr = fmt.Errorf("Bad type for '%s' - %v", k, reflect.TypeOf(v))
			} else if intVal, err := numVal.Int64(); nil != err {
				outErr = fmt.Errorf("Wrong number type for '%s' - %s", k, numVal.String())
			} else {
				uintVal := uint64(intVal)
				le.GoroutineID = &uintVal
			}
		default:

			// map data
			if nil == le.MapData {
				le.MapData = map[string]interface{}{}
			}
			le.MapData[k] = v
		}

		// If error, short-circuit
		if nil != outErr {
			return nil, outErr
		}
	}

	return &le, nil
}

// JSONToPlainText - Convert a structured JSON log line to its corresponding
// plain text representation
func JSONToPlainText(jsString string) ([]string, error) {

	if le, err := JSONToLogEntry(jsString); nil != err {
		return []string{}, err
	} else if nil == le {
		return []string{}, fmt.Errorf("Got nil pointer LogEntry")
	} else {
		formatter := StdLogFormatter{}
		return formatter.FormatEntry(*le), nil
	}
}
