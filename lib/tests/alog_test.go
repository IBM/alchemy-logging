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
	"sync"
	"testing"
	"time"

	// Third Party
	"github.com/stretchr/testify/assert"

	// Local
	"github.ibm.com/watson-discovery/alog"
)

////////////////////////////////////////////////////////////////////////////////
// Serial Tests ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Plain Text Tests ////////////////////////////////////////////////////////////

////
// Happy Path - Test the basic logging functionality:
// 1) Log with no configuration
//  -> No logs created
// 2) Log to a channel without a specific level filter
//  -> Use the default level
// 3) LUog to a channel with a specific level filter
//  -> se that filter
// 4) Log with formatting operators
//  -> Ensure formatting is properly printed
// 5) Log to a channel with a long name
//  -> Ensure channel name gets truncated correctly
////
func Test_Alog_HappyPath(t *testing.T) {

	// Set up the writer to capture logged lines
	entries := []string{}
	ConfigStdLogWriter(&entries)

	// Log with no configuration
	alog.Log("TEST", alog.INFO, "Test1")
	assert.True(t, VerifyLogs(entries, []ExpEntry{}))
	entries = []string{}

	// Set default level and try again
	alog.ConfigDefaultLevel(alog.INFO)
	alog.Log("TEST", alog.INFO, "Test2")
	alog.Log("TEST", alog.DEBUG, "Debug test!")
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "INFO", body: "Test2"},
	}))
	entries = []string{}

	// Set TEST channel back to OFF
	alog.ConfigChannel("TEST", alog.OFF)
	alog.Log("TEST", alog.INFO, "Can't see me...")
	alog.Log("FOO", alog.INFO, "Use the default!")
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "FOO  ", level: "INFO", body: "Use the default!"},
	}))
	entries = []string{}

	// Log with some formatting
	for i := 1; i < 3; i++ {
		alog.Log("FOO", alog.INFO, "This is formatting test [%d]", i)
	}
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "FOO  ", level: "INFO", body: "This is formatting test [1]"},
		ExpEntry{channel: "FOO  ", level: "INFO", body: "This is formatting test [2]"},
	}))
	entries = []string{}

	// Test a really long channel name
	alog.Log("LONGCHANNEL", alog.INFO, "Line me up nice...")
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "LONGC", level: "INFO", body: "Line me up nice..."},
	}))
	entries = []string{}

	// Reset for next test
	alog.ResetDefaults()
}

////
// Indent - Test the indentation functionality
// 1) Log with no indentation
//  -> No indentation in message
// 2) Log with one level of indentation
//  -> One level of indentation in message
// 3) Log with two levels of indentation
//  -> ...
// 4) Deindent back to one level
//  -> Log line back to one level of indentation
// 5) Deindent back to zero
//  -> ...
////
func Test_Alog_Indent(t *testing.T) {
	alog.ConfigDefaultLevel(alog.DEBUG2)

	// Set up the writer to capture logged lines
	entries := []string{}
	ConfigStdLogWriter(&entries)

	alog.Log("TEST", alog.DEBUG2, "Outside the indent")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 2")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Made it!")

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Outside the indent", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Level 1", nIndent: 1},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Level 2", nIndent: 2},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Level 1", nIndent: 1},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Made it!", nIndent: 0},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// Indent Disabled - Repeat the "Indent" test and ensure no indentation added
////
func Test_Alog_IndentDisabled(t *testing.T) {
	alog.ConfigDefaultLevel(alog.DEBUG2)

	// Set up the writer to capture logged lines
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.DisableIndent()

	alog.Log("TEST", alog.DEBUG2, "Outside the indent")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 2")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Made it!")

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Outside the indent", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Level 1", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Level 2", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Level 1", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBG2", body: "Made it!", nIndent: 0},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// Channel - Test basic functionality of ChannelLog
//
// 1) Create a ChannelLog for the "TEST" channel
// 2) Log a simple message to the channel
//  -> Message is logged with the "TEST" channel
// 3) Create and invoke a function that has its own "FOO" channel and logs in
//  the body
//  -> Message is logged with the "FOO" channel
////
func Test_Alog_Channel(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG2)

	ch := alog.UseChannel("THIS")
	ch.Log(alog.INFO, "Logged to this!")

	foo := func() {
		ch := alog.UseChannel("FOO")
		ch.Log(alog.INFO, "Doin it in FOO")
	}
	foo()

	ch.Log(alog.INFO, "Done with FOO")

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "THIS ", level: "INFO", body: "Logged to this!"},
		ExpEntry{channel: "FOO  ", level: "INFO", body: "Doin it in FOO"},
		ExpEntry{channel: "THIS ", level: "INFO", body: "Done with FOO"},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// Goroutine ID - Ensure that logging with a goroutine ID works
//
// 1) Configure with goroutine enabled
// 2) Run four goroutines simultaneously and wait for them to complete.
// 3) Log when each goroutine finishes
// 4) Log in the main goroutine once all are finished
//  -> Four log lines on PARLL from within goroutines
//  -> Four log lines on MAIN from completion
//  -> One log line on MAIN when all complete
//
// NOTE: Due to the random nature of the goroutine ID, no checks are made on the
//  values of the GIDs.
////
func Test_Alog_GID(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG3)
	alog.EnableGID()
	ch := alog.UseChannel("MAIN")

	waitChans := []chan bool{
		make(chan bool),
		make(chan bool),
		make(chan bool),
		make(chan bool),
	}
	f := func(i int) {
		ch := alog.UseChannel("PARLL")
		ch.Log(alog.INFO, "Logging in a goroutine")
		time.Sleep(time.Duration(i) * time.Millisecond)
		waitChans[i] <- true
	}
	for i := 0; i < 4; i++ {
		go f(i)
	}
	for i := 0; i < 4; i++ {
		<-waitChans[i]
		ch.Log(alog.INFO, "Done with %d", i)
	}
	ch.Log(alog.INFO, "All goroutines done")

	// Check the result
	assert.True(t, VerifyLogsUnordered(entries, []ExpEntry{
		ExpEntry{channel: "PARLL", level: "INFO", body: "Logging in a goroutine", hasGid: true},
		ExpEntry{channel: "PARLL", level: "INFO", body: "Logging in a goroutine", hasGid: true},
		ExpEntry{channel: "PARLL", level: "INFO", body: "Logging in a goroutine", hasGid: true},
		ExpEntry{channel: "PARLL", level: "INFO", body: "Logging in a goroutine", hasGid: true},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "Done with 0", hasGid: true},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "Done with 1", hasGid: true},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "Done with 2", hasGid: true},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "Done with 3", hasGid: true},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "All goroutines done", hasGid: true},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// IsEnabled - Test the functionality of the IsEnabled function
//
// 1) Log outside check
//  -> Logged
// 2) Log on INFO inside enabled check on DEBUG
//  -> Hidden due to check
// 3) Log on INFO inside enabled check on INFO
//  -> Logged
// 4) Log inside check on DEBUG3 on a special channel with DEBUG4 enabled
//  -> Logged
////
func Test_Alog_IsEnabled(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.Config(alog.INFO, alog.ChannelMap{"HIGH": alog.DEBUG4})
	ch := alog.UseChannel("MAIN")

	ch.Log(alog.INFO, "About to check if enabled")
	if ch.IsEnabled(alog.DEBUG) {
		ch.Log(alog.INFO, "You can't see me even though I'm on INFO")
	}
	if ch.IsEnabled(alog.INFO) {
		ch.Log(alog.INFO, "HELLO!")
	}
	if alog.IsEnabled("HIGH", alog.DEBUG3) {
		alog.Log("HIGH", alog.DEBUG3, "Deep debugging time!")
	}

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "MAIN ", level: "INFO", body: "About to check if enabled"},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "HELLO!"},
		ExpEntry{channel: "HIGH ", level: "DBG3", body: "Deep debugging time!"},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// Scope - Test the functionality of the LogScope
//
// 1) Use the defer alog.LogScope(...).Close() paradigm within a local function
//  -> Log a Start and End block around the logging in the internal function
// 2) Log outside of local function
//  -> Ensure comes after End block
// 3) Use defer alog.LogScope(...).Close() paradigm for the test itself
//  -> Start block logged, but End not logged before check
// 4) Log after test scope
//  -> Log line shows up after Start block
////
func Test_Alog_Scope(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG)
	ch := alog.UseChannel("MAIN")

	// Emulate a "local scope" using an anonymous function
	ch.Log(alog.INFO, "Trying a scope now...")
	func() {
		defer alog.LogScope("TEST", alog.INFO, "Local scope").Close()
		alog.Log("TEST", alog.INFO, "inside the scope")
	}()
	ch.Log(alog.INFO, "Bye bye scope")

	// Start/end a block for this test as a function
	defer ch.LogScope(alog.DEBUG, "End of function scope").Close()
	ch.Log(alog.DEBUG, "Got something to say??")

	// Check the result
	// NOTE: End of "End of function scope" won't have logged yet because it waits
	//  for the function to close
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "MAIN ", level: "INFO", body: "Trying a scope now...", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "INFO", body: "Start: Local scope", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "INFO", body: "inside the scope", nIndent: 1},
		ExpEntry{channel: "TEST ", level: "INFO", body: "End: Local scope", nIndent: 0},
		ExpEntry{channel: "MAIN ", level: "INFO", body: "Bye bye scope", nIndent: 0},
		ExpEntry{channel: "MAIN ", level: "DBUG", body: "Start: End of function scope", nIndent: 0},
		ExpEntry{channel: "MAIN ", level: "DBUG", body: "Got something to say??", nIndent: 1},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

func freeFuncTest() {
	ch := alog.UseChannel("FREE")
	defer ch.DetailFnLog(alog.DEBUG, "").Close()
	ch.Log(alog.INFO, "Something soooooo cool!")
	ch.Log(alog.DEBUG4, "Hide all the super details")
}

////
// ChFnLog - Test the function trace logging functionality
//
// 1) Log simple line
//  -> Message logged
// 2) Use FnLog on a LogChannel in a locally declared function
//  -> Start/End block for function and indented messages inside
// 3) Call free function which does FnLog
//  -> Start/End block for function and indented messages inside
// 4) Log after all functions
//  -> Message appears after End block
////
func Test_Alog_ChFnLog(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG)
	ch := alog.UseChannel("FNLOG")

	ch.Log(alog.INFO, "Let's get started...")
	foo := 0
	f := func() {
		defer ch.FnLog("%d", foo).Close()
		bar := 1
		bat := 2
		ch.Log(alog.INFO, "bar: %d", bar)
		ch.Log(alog.INFO, "bat: %d", bat)
	}
	f()
	freeFuncTest()
	ch.Log(alog.INFO, "... and we're done!")

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "FNLOG", level: "INFO", body: "Let's get started...", nIndent: 0},
		ExpEntry{channel: "FNLOG", level: "TRCE", body: "Start: func1(0)", nIndent: 0},
		ExpEntry{channel: "FNLOG", level: "INFO", body: "bar: 1", nIndent: 1},
		ExpEntry{channel: "FNLOG", level: "INFO", body: "bat: 2", nIndent: 1},
		ExpEntry{channel: "FNLOG", level: "TRCE", body: "End: func1(0)", nIndent: 0},
		ExpEntry{channel: "FREE ", level: "DBUG", body: "Start: freeFuncTest()", nIndent: 0},
		ExpEntry{channel: "FREE ", level: "INFO", body: "Something soooooo cool!", nIndent: 1},
		ExpEntry{channel: "FREE ", level: "DBUG", body: "End: freeFuncTest()", nIndent: 0},
		ExpEntry{channel: "FNLOG", level: "INFO", body: "... and we're done!", nIndent: 0},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// FnLog - Test the function trace logging functionality without a channel
//
// 1) Log simple line
//  -> Message logged
// 2) Use FnLog in a locally declared function
//  -> Start/End block for function and indented messages inside
// 3) Use DetailFnLog in a locally declared function
//  -> Start/End block for function and indented messages inside
////
func Test_Alog_FnLog(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigStdLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG)

	alog.Log("TEST", alog.INFO, "Let's get started...")
	foo := 0
	f1 := func() {
		defer alog.FnLog("TEST", "%d", foo).Close()
		bar := 1
		bat := 2
		alog.Log("TEST", alog.INFO, "bar: %d", bar)
		alog.Log("TEST", alog.INFO, "bat: %d", bat)
	}
	f2 := func() {
		defer alog.DetailFnLog("TEST", alog.DEBUG, "").Close()
	}
	f1()
	f2()
	alog.Log("TEST", alog.INFO, "... and we're done!")

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "INFO", body: "Let's get started...", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "TRCE", body: "Start: func1(0)", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "INFO", body: "bar: 1", nIndent: 1},
		ExpEntry{channel: "TEST ", level: "INFO", body: "bat: 2", nIndent: 1},
		ExpEntry{channel: "TEST ", level: "TRCE", body: "End: func1(0)", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBUG", body: "Start: func2()", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "DBUG", body: "End: func2()", nIndent: 0},
		ExpEntry{channel: "TEST ", level: "INFO", body: "... and we're done!", nIndent: 0},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// ServiceName - Test ServiceName functionality with the standard logger
//
// 1) Set a service name
// 2) Log a simple line
//  -> Verify service name is set in message header
////
func Test_Alog_ServiceName(t *testing.T) {
	alog.ConfigDefaultLevel(alog.DEBUG2)

	// Set up the writer to capture logged lines
	entries := []string{}
	ConfigStdLogWriter(&entries)
	sn := "test_service"
	alog.SetServiceName(sn)

	alog.Log("TEST", alog.INFO, "Hi there")

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "INFO", body: "Hi there", servicename: &sn},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// LogMap - Test structured map data logging
//
// 1) Log a LogMap line
//  -> Verify key/val output
////
func Test_Alog_LogMap(t *testing.T) {
	alog.ConfigDefaultLevel(alog.DEBUG2)

	// Set up the writer to capture logged lines
	entries := []string{}
	ConfigStdLogWriter(&entries)

	alog.LogMap("TEST", alog.INFO, map[string]interface{}{
		"b": 1,
		"a": "two",
		"c": []string{"e", "f"},
	})

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "INFO", body: "a: two"},
		ExpEntry{channel: "TEST ", level: "INFO", body: "b: 1"},
		ExpEntry{channel: "TEST ", level: "INFO", body: "c: [e f]"},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// LogWithMap - Test message plus structured data
//
// 1) Log a LogWithMap line
//  -> Verify key/val output
////
func Test_Alog_LogWithMap(t *testing.T) {
	alog.ConfigDefaultLevel(alog.DEBUG2)

	// Set up the writer to capture logged lines
	entries := []string{}
	ConfigStdLogWriter(&entries)

	alog.LogWithMap("TEST", alog.INFO, map[string]interface{}{
		"b": 1,
		"a": "two",
		"c": []string{"e", "f"},
	}, "Hi logging world, this is a test %d", 1)

	// Check the result
	assert.True(t, VerifyLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST ", level: "INFO", body: "Hi logging world, this is a test 1"},
		ExpEntry{channel: "TEST ", level: "INFO", body: "a: two"},
		ExpEntry{channel: "TEST ", level: "INFO", body: "b: 1"},
		ExpEntry{channel: "TEST ", level: "INFO", body: "c: [e f]"},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

// JSON Tests //////////////////////////////////////////////////////////////////

////
// JSON Basic Channel - Verify basic functionality of JSON formatter using
//  LogChannel
//
// 1) Configure to use JSON formatter
// 2) Log a formatted line that should print
//  -> Log line parses as JSON and is correctly formatted
// 3) Log on a disabled level
//  -> Line not logged
////
func Test_Alog_JSONBasicChannel(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG)
	ch := alog.UseChannel("MAIN")

	// Log a simple line on info
	ch.Log(alog.INFO, "[%d] This is a formatted test", 10)

	// Log a line on debug3 that shouldn't show up
	ch.Log(alog.DEBUG3, "This shouldn't show up")

	// Check the result
	assert.True(t, len(entries) == 1)
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "MAIN", level: "info", body: "[10] This is a formatted test"},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON Happy Path - Repeat the HappyPath test from the Std formatter with JSON
//
// 1) Log with no configuration
//  -> No logs created
// 2) Log to a channel without a specific level filter
//  -> Use the default level
// 3) LUog to a channel with a specific level filter
//  -> se that filter
// 4) Log with formatting operators
//  -> Ensure formatting is properly printed
// 5) Log to a channel with a long name
//  -> Ensure channel name gets truncated correctly
////
func Test_Alog_JSONHappyPath(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)

	// Log with no configuration
	alog.Log("TEST", alog.INFO, "Test1")
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{}))
	entries = []string{}

	// Set default level and try again
	alog.ConfigDefaultLevel(alog.INFO)
	alog.Log("TEST", alog.INFO, "Test2")
	alog.Log("TEST", alog.DEBUG, "Debug test!")
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST", level: "info", body: "Test2"},
	}))
	entries = []string{}

	// Set TEST channel back to OFF
	alog.ConfigChannel("TEST", alog.OFF)
	alog.Log("TEST", alog.INFO, "Can't see me...")
	alog.Log("FOO", alog.INFO, "Use the default!")
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "FOO", level: "info", body: "Use the default!"},
	}))
	entries = []string{}

	// Log with some formatting
	for i := 1; i < 3; i++ {
		alog.Log("FOO", alog.INFO, "This is formatting test [%d]", i)
	}
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "FOO", level: "info", body: "This is formatting test [1]"},
		ExpEntry{channel: "FOO", level: "info", body: "This is formatting test [2]"},
	}))
	entries = []string{}

	// Test a really long channel name (don't truncate for JSON)
	alog.Log("LONGCHANNEL", alog.INFO, "Line me up nice...")
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "LONGCHANNEL", level: "info", body: "Line me up nice..."},
	}))
	entries = []string{}

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON Service Name - Test ServiceName functionality with JSON output
//
// 1) Configure a service name
// 2) Log an enabled message
//  -> Logged with service name
// 3) Log a disabled message
//  -> Not logged
////
func Test_Alog_JSONServiceName(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	sn := "test_service"
	alog.SetServiceName(sn)
	alog.ConfigDefaultLevel(alog.DEBUG)
	ch := alog.UseChannel("MAIN")

	// Log a simple line on info
	ch.Log(alog.INFO, "[%d] This is a formatted test", 10)

	// Log a line on debug3 that shouldn't show up
	ch.Log(alog.DEBUG3, "This shouldn't show up")

	// Check the result
	assert.True(t, len(entries) == 1)
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "MAIN", level: "info", body: "[10] This is a formatted test", servicename: &sn},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON Indent - Verify indentation counting with JSON output
//
// 1) Log with no indentation
//  -> No indentation in message
// 2) Log with one level of indentation
//  -> One level of indentation in message
// 3) Log with two levels of indentation
//  -> ...
// 4) Deindent back to one level
//  -> Log line back to one level of indentation
// 5) Deindent back to zero
//  -> ...
////
func Test_Alog_JSONIndent(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG2)

	alog.Log("TEST", alog.DEBUG2, "Outside the indent")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 2")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Made it!")

	// Check the result
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST", level: "debug2", body: "Outside the indent", nIndent: 0},
		ExpEntry{channel: "TEST", level: "debug2", body: "Level 1", nIndent: 1},
		ExpEntry{channel: "TEST", level: "debug2", body: "Level 2", nIndent: 2},
		ExpEntry{channel: "TEST", level: "debug2", body: "Level 1", nIndent: 1},
		ExpEntry{channel: "TEST", level: "debug2", body: "Made it!", nIndent: 0},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON Indent Disabled - Verify disabling indentation works with JSON
//
// n) Repeat the above test and make sure no indentation
////
func Test_Alog_JSONIndentDisabled(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG2)
	alog.DisableIndent()

	alog.Log("TEST", alog.DEBUG2, "Outside the indent")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Indent()
	alog.Log("TEST", alog.DEBUG2, "Level 2")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Level 1")
	alog.Deindent()
	alog.Log("TEST", alog.DEBUG2, "Made it!")

	// Check the result
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST", level: "debug2", body: "Outside the indent", nIndent: 0},
		ExpEntry{channel: "TEST", level: "debug2", body: "Level 1", nIndent: 0},
		ExpEntry{channel: "TEST", level: "debug2", body: "Level 2", nIndent: 0},
		ExpEntry{channel: "TEST", level: "debug2", body: "Level 1", nIndent: 0},
		ExpEntry{channel: "TEST", level: "debug2", body: "Made it!", nIndent: 0},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON LogMap - Verify that LogMap k/v entries are serialized
//
// 1) Log a LogMap line
//  -> map data keys/values present in result
////
func Test_Alog_JSONLogMap(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG2)

	// NOTE: The 'c' key works as a []string as well, but the validation fails on
	// reflect.DeepEqual since you have one []string and one []interface{}
	md := map[string]interface{}{
		"test_key":  "string_val",
		"test_key2": 23,
		"c":         []interface{}{string("e"), string("f")},
	}
	alog.LogMap("TEST", alog.DEBUG2, md)

	// Check the result
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST", level: "debug2", mapData: md},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON LogWithMap - Verify that LogWithMap k/v entries are serialized along
// with full message
//
// 1) Log a LogWithMap line
//  -> map data keys/values present in result
//  -> formatted message present in result
////
func Test_Alog_JSONLogWithMap(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG2)

	md := map[string]interface{}{
		"test_key":  "string_val",
		"test_key2": 23,
	}
	alog.LogWithMap("TEST", alog.DEBUG2, md, "Hello logging world, this is a test %d", 1)

	// Check the result
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST", level: "debug2", mapData: md, body: "Hello logging world, this is a test 1"},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////
// JSON GID - Verify that the goroutine id is handled correctly
//
// 1) Enable GID
// 2) Log a line
//  -> Line contains gid
////
func Test_Alog_JSONGID(t *testing.T) {

	// Configure
	entries := []string{}
	ConfigJSONLogWriter(&entries)
	alog.ConfigDefaultLevel(alog.DEBUG2)
	alog.EnableGID()

	alog.Log("TEST", alog.DEBUG2, "Test with GID")

	// Check the result
	assert.True(t, VerifyJSONLogs(entries, []ExpEntry{
		ExpEntry{channel: "TEST", level: "debug2", body: "Test with GID", hasGid: true},
	}))

	// Reset for next test
	alog.ResetDefaults()
}

////////////////////////////////////////////////////////////////////////////////
// Parallel Tests //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// These tests are run in parallel to sniff out any race conditions or other
// thread-safety issues. As such, they don't bother checking results.

var parallelConfigured = false
var parallelConfigMu = sync.Mutex{}

func configParallelTests() {
	parallelConfigMu.Lock()
	if !parallelConfigured {
		alog.Config(alog.INFO, alog.ChannelMap{
			"MID":  alog.DEBUG,
			"HIGH": alog.DEBUG4,
		})
		alog.EnableGID()
		parallelConfigured = true
	}
	parallelConfigMu.Unlock()
}

func Test_Alog_Parallel_Basic(t *testing.T) {
	t.Parallel()
	configParallelTests()
	alog.Log("TEST", alog.INFO, "Hi there")
	alog.Log("TEST", alog.DEBUG, "Invisible")
	alog.Log("HIGH", alog.DEBUG4, "Down in the weeds")
}

func Test_Alog_Parallel_Channel(t *testing.T) {
	t.Parallel()
	configParallelTests()
	ch := alog.UseChannel("MID")
	ch.Log(alog.INFO, "See me on INFO")
	ch.Log(alog.DEBUG, "See me on DEBUG")
	ch.Log(alog.DEBUG2, "Don't see me on DEBUG2")
}

func Test_Alog_Parallel_FnLog(t *testing.T) {
	t.Parallel()
	configParallelTests()
	ch := alog.UseChannel("MID")
	f1 := func() {
		defer ch.FnLog("").Close()
		ch.Log(alog.INFO, "Something in f1")
	}
	f2 := func() {
		defer ch.DetailFnLog(alog.DEBUG, "").Close()
		ch.Log(alog.INFO, "Something in f2")
		ch.Log(alog.DEBUG, "Something else in f2")
	}
	f1()
	f2()
	f1()
}
