// *************************************************
// 5737-C06
// (C) Copyright IBM Corp. 2017 All Rights Reserved.
// The source code for this program is not published or otherwise
// divested of its trade secrets, irrespective of what has been
// deposited with the U.S. Copyright Office.
// *************************************************

package alogtest

import (
	// Standard
	"fmt"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	// Third Party
	"github.com/stretchr/testify/assert"

	// Local
	alog "github.ibm.com/watson-discovery/alog/lib"
)

// Tests - General Helpers /////////////////////////////////////////////////////

////
// LevelFromString
// 1) Test each valid level string
//  -> Valid level value and no error
// 2) Capital letters
//  -> ERROR level with error returned
// 3) Bad name
//  -> ERROR level with error returned
// 4) Header str representation
//  -> ERROR level with error returned
////
func Test_AlogExtras_LevelFromString(t *testing.T) {

	// Set up logging
	alog.Config(alog.TRACE, alog.ChannelMap{})
	defer alog.ResetDefaults()
	defer alog.FnLog("TEST", "").Close()

	// Valid levels
	{
		lvl, err := alog.LevelFromString("off")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.OFF)
	}
	{
		lvl, err := alog.LevelFromString("fatal")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.FATAL)
	}
	{
		lvl, err := alog.LevelFromString("error")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.ERROR)
	}
	{
		lvl, err := alog.LevelFromString("warning")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.WARNING)
	}
	{
		lvl, err := alog.LevelFromString("info")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.INFO)
	}
	{
		lvl, err := alog.LevelFromString("trace")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.TRACE)
	}
	{
		lvl, err := alog.LevelFromString("debug")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.DEBUG)
	}
	{
		lvl, err := alog.LevelFromString("debug1")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.DEBUG1)
	}
	{
		lvl, err := alog.LevelFromString("debug2")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.DEBUG2)
	}
	{
		lvl, err := alog.LevelFromString("debug3")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.DEBUG3)
	}
	{
		lvl, err := alog.LevelFromString("debug4")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, alog.DEBUG4)
	}

	// Invalid levels
	{
		lvl, err := alog.LevelFromString("OFF")
		assert.NotEqual(t, err, nil)
		assert.Equal(t, lvl, alog.ERROR)
	}
	{
		lvl, err := alog.LevelFromString("foobar")
		assert.NotEqual(t, err, nil)
		assert.Equal(t, lvl, alog.ERROR)
	}
	{
		lvl, err := alog.LevelFromString("DBG4")
		assert.NotEqual(t, err, nil)
		assert.Equal(t, lvl, alog.ERROR)
	}
}

////
// ParseChannelFilter
// 1) Valid filter spec
//  -> Correctly parses, no error
// 2) Invalid list format (missing ',' separator)
//  -> Parse fails with error
// 3) Invalid filter format (';' instead of ':' separator)
//  -> Parse fails with error
// 4) Invalid level
//  -> Parse fails with error
////
func Test_AlogExtras_ParseChannelFilter(t *testing.T) {

	// Set up logging
	alog.Config(alog.TRACE, alog.ChannelMap{})
	defer alog.ResetDefaults()
	defer alog.FnLog("TEST", "").Close()

	// Valid filter spec
	{
		spec := "MAIN:debug,TEST:debug3"
		m, e := alog.ParseChannelFilter(spec)
		assert.Equal(t, e, nil)
		assert.True(t, ValidateChannelMap(m, alog.ChannelMap{
			"MAIN": alog.DEBUG,
			"TEST": alog.DEBUG3,
		}))
	}

	// Invalid list format
	// -> Fail to parse map at all
	{
		spec := "MAIN:debugTEST:debug3"
		m, e := alog.ParseChannelFilter(spec)
		assert.NotEqual(t, e, nil)
		assert.Equal(t, len(m), 0)
	}

	// Invalid pair format
	// -> Correctly parse the two valid entries
	{
		spec := "MAIN:debug,TEST;debug3,FOO:info"
		m, e := alog.ParseChannelFilter(spec)
		assert.NotEqual(t, e, nil)
		assert.Equal(t, len(m), 0)
	}

	// Invalid level
	// -> Correctly parse the two valid entries
	{
		spec := "MAIN:dummy,TEST:debug3"
		m, e := alog.ParseChannelFilter(spec)
		assert.NotEqual(t, e, nil)
		assert.Equal(t, len(m), 0)
	}
}

// Tests - Command Line Flags //////////////////////////////////////////////////

////
// ConfigureFromFlags
// 1) Create FlagSet with manual values
// 2) Run configuration
// 3) Make sure configuration is correct
////
func Test_AlogExtras_ConfigureFromFlags(t *testing.T) {

	// Set up flags manually
	fs := alog.FlagSet{
		DefaultLevel:     new(string),
		ChannelConfig:    new(string),
		ChannelHeaderLen: new(int),
		EnableGID:        new(bool),
		EnableFuncSig:    new(bool),
		DisableIndent:    new(bool),
		ServiceName:      new(string),
		OutputJSON:       new(bool),
	}
	*(fs.DefaultLevel) = "info"
	*(fs.ChannelConfig) = "TEST:debug,DEEP:debug4"
	*(fs.ChannelHeaderLen) = 5
	*(fs.EnableGID) = true
	*(fs.EnableFuncSig) = false
	*(fs.DisableIndent) = false
	*(fs.ServiceName) = "test_service"
	*(fs.OutputJSON) = false

	// Configure
	err := alog.ConfigureFromFlags(fs)
	assert.Equal(t, nil, err)
	defer alog.ResetDefaults()

	// Validate
	assert.Equal(t, alog.GetDefaultLevel(), alog.INFO)
	cm := alog.GetChannelMap()
	assert.True(t, ValidateChannelMap(cm, alog.ChannelMap{
		"TEST": alog.DEBUG,
		"DEEP": alog.DEBUG4,
	}))
	assert.Equal(t, alog.GetServiceName(), "test_service")
	assert.Equal(t, alog.GetChannelHeaderLen(), 5)
	assert.True(t, alog.GIDEnabled())
	assert.True(t, alog.IndentEnabled())
	assert.False(t, alog.FuncSigEnabled())
}

// Tests - Dynamic Config //////////////////////////////////////////////////////

////
// ConfigureDynamicLogging - Permanent
// 1) Set up a config object with no timeout
// 2) Use ConfigureDynamicLogging
// 3) Validate configuration
////
func Test_AlogExtras_ConfigureDynamicLogging_Permanent(t *testing.T) {

	// Set up config object
	cfg := alog.DynamicLogConfig{
		DefaultLevel: "info",
		Filters:      "TEST:debug,DEEP:debug4",
	}

	// Configure
	err := alog.ConfigureDynamicLogging(cfg)
	defer alog.ResetDefaults()
	assert.Equal(t, err, nil)

	// Validate
	assert.Equal(t, alog.GetDefaultLevel(), alog.INFO)
	cm := alog.GetChannelMap()
	assert.True(t, ValidateChannelMap(cm, alog.ChannelMap{
		"TEST": alog.DEBUG,
		"DEEP": alog.DEBUG4,
	}))
}

////
// ConfigureDynamicLogging - Temporary
// 1) Configure directly
// 2) Set up a config object with a timeout
// 3) Use ConfigureDynamicLogging
// 4) Validate configuration
// 5) Use ConfigureDynamicLogging before timeout
//  -> error
// 6) Wait for timeout
// 7) Validate original configuration
// 8) Use ConfigureDynamicLogging again without timeout
//  -> no error
////
func Test_AlogExtras_ConfigureDynamicLogging_Temporary(t *testing.T) {

	// Configure directly
	alog.Config(alog.TRACE, alog.ChannelMap{"TEST": alog.INFO})
	defer alog.ResetDefaults()

	// Validate direct config
	assert.Equal(t, alog.GetDefaultLevel(), alog.TRACE)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{"TEST": alog.INFO}))

	// Set up config object with timeout
	timeout := uint32(1)
	cfg := alog.DynamicLogConfig{
		DefaultLevel: "info",
		Filters:      "TEST:debug,DEEP:debug4",
		Timeout:      timeout,
	}

	// Configure
	assert.Equal(t, alog.ConfigureDynamicLogging(cfg), nil)

	// Validate temporary config
	assert.Equal(t, alog.GetDefaultLevel(), alog.INFO)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{
		"TEST": alog.DEBUG,
		"DEEP": alog.DEBUG4,
	}))

	// Try second dynamic config and make sure error
	assert.NotEqual(t, alog.ConfigureDynamicLogging(cfg), nil)

	// Wait for timeout
	time.Sleep((time.Duration(timeout) + 1) * time.Second)

	// Validate original config reset
	assert.Equal(t, alog.GetDefaultLevel(), alog.TRACE)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{"TEST": alog.INFO}))

	// Rerun dynamic config without a timeout and make sure no error returned
	cfg.Timeout = 0
	assert.Equal(t, alog.ConfigureDynamicLogging(cfg), nil)
}

////
// DynamicHandler
// 1) Fake up an http.ResponseWriter and http.Request
// 2) Invoke DynamicHandler
// 3) Validate configuration
////
func Test_AlogExtras_DynamicHandler(t *testing.T) {

	// Set up logging
	alog.Config(alog.DEBUG, alog.ChannelMap{})
	defer alog.ResetDefaults()
	defer alog.FnLog("TEST", "").Close()

	// Fake up http objects
	timeout := uint32(1)
	writer := httptest.NewRecorder()
	request := httptest.NewRequest(
		"GET",
		fmt.Sprintf("http://localhost:54321?default_level=info&filters=TEST:debug,DEEP:debug4&timeout=%d", timeout),
		strings.NewReader(""),
	)

	// Invoke DynamicHandler
	alog.DynamicHandler(writer, request)

	// Validate temporary config
	assert.Equal(t, alog.GetDefaultLevel(), alog.INFO)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{
		"TEST": alog.DEBUG,
		"DEEP": alog.DEBUG4,
	}))

	// Wait for timeout
	time.Sleep((time.Duration(timeout) + 1) * time.Second)

	// Validate original config reset
	assert.Equal(t, alog.GetDefaultLevel(), alog.DEBUG)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{}))
}

////
// ConfigureDynamicLogging - Bad DefaultLevel
// 1) Set up a config object with a bad default level string
// 2) Use ConfigureDynamicLogging
//  -> error
// 3) Validate configuration unchanged
////
func Test_AlogExtras_ConfigureDynamicLogging_BadDefaultLevel(t *testing.T) {

	// Set up base logging
	alog.Config(alog.DEBUG, alog.ChannelMap{})
	defer alog.ResetDefaults()
	defer alog.FnLog("TEST", "").Close()

	// Set up config object
	cfg := alog.DynamicLogConfig{
		DefaultLevel: "foobar",
		Filters:      "TEST:debug,DEEP:debug4",
	}

	// Configure
	err := alog.ConfigureDynamicLogging(cfg)
	defer alog.ResetDefaults()
	assert.NotEqual(t, err, nil)

	// Validate unchanged
	assert.Equal(t, alog.GetDefaultLevel(), alog.DEBUG)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{}))
}

////
// ConfigureDynamicLogging - Bad ChannelMap
// 1) Set up a config object with a bad channel map string
// 2) Use ConfigureDynamicLogging
//  -> error
// 3) Validate configuration unchanged
////
func Test_AlogExtras_ConfigureDynamicLogging_BadChannelMap(t *testing.T) {

	// Set up base logging
	alog.Config(alog.DEBUG, alog.ChannelMap{})
	defer alog.ResetDefaults()
	defer alog.FnLog("TEST", "").Close()

	// Set up config object
	cfg := alog.DynamicLogConfig{
		DefaultLevel: "info",
		Filters:      "TEST:debugDEEP:debug4",
	}

	// Configure
	err := alog.ConfigureDynamicLogging(cfg)
	defer alog.ResetDefaults()
	assert.NotEqual(t, err, nil)

	// Validate unchanged
	assert.Equal(t, alog.GetDefaultLevel(), alog.DEBUG)
	assert.True(t, ValidateChannelMap(alog.GetChannelMap(), alog.ChannelMap{}))
}
