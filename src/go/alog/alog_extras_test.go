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
	// Standard
	"fmt"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	// Third Party
	"github.com/stretchr/testify/assert"
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
	Config(TRACE, ChannelMap{})
	defer ResetDefaults()
	defer FnLog("TEST", "").Close()

	// Valid levels
	{
		lvl, err := LevelFromString("off")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, OFF)
	}
	{
		lvl, err := LevelFromString("fatal")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, FATAL)
	}
	{
		lvl, err := LevelFromString("error")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, ERROR)
	}
	{
		lvl, err := LevelFromString("warning")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, WARNING)
	}
	{
		lvl, err := LevelFromString("info")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, INFO)
	}
	{
		lvl, err := LevelFromString("trace")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, TRACE)
	}
	{
		lvl, err := LevelFromString("debug")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, DEBUG)
	}
	{
		lvl, err := LevelFromString("debug1")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, DEBUG1)
	}
	{
		lvl, err := LevelFromString("debug2")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, DEBUG2)
	}
	{
		lvl, err := LevelFromString("debug3")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, DEBUG3)
	}
	{
		lvl, err := LevelFromString("debug4")
		assert.Equal(t, err, nil)
		assert.Equal(t, lvl, DEBUG4)
	}

	// Invalid levels
	{
		lvl, err := LevelFromString("OFF")
		assert.NotEqual(t, err, nil)
		assert.Equal(t, lvl, ERROR)
	}
	{
		lvl, err := LevelFromString("foobar")
		assert.NotEqual(t, err, nil)
		assert.Equal(t, lvl, ERROR)
	}
	{
		lvl, err := LevelFromString("DBG4")
		assert.NotEqual(t, err, nil)
		assert.Equal(t, lvl, ERROR)
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
	Config(TRACE, ChannelMap{})
	defer ResetDefaults()
	defer FnLog("TEST", "").Close()

	// Valid filter spec
	{
		spec := "MAIN:debug,TEST:debug3"
		m, e := ParseChannelFilter(spec)
		assert.Equal(t, e, nil)
		assert.True(t, ValidateChannelMap(m, ChannelMap{
			"MAIN": DEBUG,
			"TEST": DEBUG3,
		}))
	}

	// Invalid list format
	// -> Fail to parse map at all
	{
		spec := "MAIN:debugTEST:debug3"
		m, e := ParseChannelFilter(spec)
		assert.NotEqual(t, e, nil)
		assert.Equal(t, len(m), 0)
	}

	// Invalid pair format
	// -> Correctly parse the two valid entries
	{
		spec := "MAIN:debug,TEST;debug3,FOO:info"
		m, e := ParseChannelFilter(spec)
		assert.NotEqual(t, e, nil)
		assert.Equal(t, len(m), 0)
	}

	// Invalid level
	// -> Correctly parse the two valid entries
	{
		spec := "MAIN:dummy,TEST:debug3"
		m, e := ParseChannelFilter(spec)
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
	defaultLevel := "info"
	channelConfig := "TEST:debug,DEEP:debug4"
	channelHeaderLen := 5
	enableGID := true
	enableFuncSig := false
	disableIndent := false
	serviceName := "test_service"
	outputJSON := false
	fs := FlagSet{
		DefaultLevel:     &defaultLevel,
		ChannelConfig:    &channelConfig,
		ChannelHeaderLen: &channelHeaderLen,
		EnableGID:        &enableGID,
		EnableFuncSig:    &enableFuncSig,
		DisableIndent:    &disableIndent,
		ServiceName:      &serviceName,
		OutputJSON:       &outputJSON,
	}

	// Configure
	err := ConfigureFromFlags(fs)
	assert.Equal(t, nil, err)
	defer ResetDefaults()

	// Validate
	assert.Equal(t, GetDefaultLevel(), INFO)
	cm := GetChannelMap()
	assert.True(t, ValidateChannelMap(cm, ChannelMap{
		"TEST": DEBUG,
		"DEEP": DEBUG4,
	}))
	assert.Equal(t, GetServiceName(), "test_service")
	assert.Equal(t, GetChannelHeaderLen(), 5)
	assert.True(t, GIDEnabled())
	assert.True(t, IndentEnabled())
	assert.False(t, FuncSigEnabled())
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
	cfg := DynamicLogConfig{
		DefaultLevel: "info",
		Filters:      "TEST:debug,DEEP:debug4",
	}

	// Configure
	err := ConfigureDynamicLogging(cfg)
	defer ResetDefaults()
	assert.Equal(t, err, nil)

	// Validate
	assert.Equal(t, GetDefaultLevel(), INFO)
	cm := GetChannelMap()
	assert.True(t, ValidateChannelMap(cm, ChannelMap{
		"TEST": DEBUG,
		"DEEP": DEBUG4,
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
	Config(TRACE, ChannelMap{"TEST": INFO})
	defer ResetDefaults()

	// Validate direct config
	assert.Equal(t, GetDefaultLevel(), TRACE)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{"TEST": INFO}))

	// Set up config object with timeout
	timeout := uint32(1)
	cfg := DynamicLogConfig{
		DefaultLevel: "info",
		Filters:      "TEST:debug,DEEP:debug4",
		Timeout:      timeout,
	}

	// Configure
	assert.Equal(t, ConfigureDynamicLogging(cfg), nil)

	// Validate temporary config
	assert.Equal(t, GetDefaultLevel(), INFO)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{
		"TEST": DEBUG,
		"DEEP": DEBUG4,
	}))

	// Try second dynamic config and make sure error
	assert.NotEqual(t, ConfigureDynamicLogging(cfg), nil)

	// Wait for timeout
	time.Sleep((time.Duration(timeout) + 1) * time.Second)

	// Validate original config reset
	assert.Equal(t, GetDefaultLevel(), TRACE)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{"TEST": INFO}))

	// Rerun dynamic config without a timeout and make sure no error returned
	cfg.Timeout = 0
	assert.Equal(t, ConfigureDynamicLogging(cfg), nil)
}

////
// DynamicHandler
// 1) Fake up an http.ResponseWriter and http.Request
// 2) Invoke DynamicHandler
// 3) Validate configuration
////
func Test_AlogExtras_DynamicHandler(t *testing.T) {

	// Set up logging
	Config(DEBUG, ChannelMap{})
	defer ResetDefaults()
	defer FnLog("TEST", "").Close()

	// Fake up http objects
	timeout := uint32(1)
	writer := httptest.NewRecorder()
	request := httptest.NewRequest(
		"GET",
		fmt.Sprintf("http://localhost:54321?default_level=info&filters=TEST:debug,DEEP:debug4&timeout=%d", timeout),
		strings.NewReader(""),
	)

	// Invoke DynamicHandler
	DynamicHandler(writer, request)

	// Validate temporary config
	assert.Equal(t, GetDefaultLevel(), INFO)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{
		"TEST": DEBUG,
		"DEEP": DEBUG4,
	}))

	// Wait for timeout
	time.Sleep((time.Duration(timeout) + 1) * time.Second)

	// Validate original config reset
	assert.Equal(t, GetDefaultLevel(), DEBUG)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{}))
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
	Config(DEBUG, ChannelMap{})
	defer ResetDefaults()
	defer FnLog("TEST", "").Close()

	// Set up config object
	cfg := DynamicLogConfig{
		DefaultLevel: "foobar",
		Filters:      "TEST:debug,DEEP:debug4",
	}

	// Configure
	err := ConfigureDynamicLogging(cfg)
	defer ResetDefaults()
	assert.NotEqual(t, err, nil)

	// Validate unchanged
	assert.Equal(t, GetDefaultLevel(), DEBUG)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{}))
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
	Config(DEBUG, ChannelMap{})
	defer ResetDefaults()
	defer FnLog("TEST", "").Close()

	// Set up config object
	cfg := DynamicLogConfig{
		DefaultLevel: "info",
		Filters:      "TEST:debugDEEP:debug4",
	}

	// Configure
	err := ConfigureDynamicLogging(cfg)
	defer ResetDefaults()
	assert.NotEqual(t, err, nil)

	// Validate unchanged
	assert.Equal(t, GetDefaultLevel(), DEBUG)
	assert.True(t, ValidateChannelMap(GetChannelMap(), ChannelMap{}))
}
