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
	"encoding/json"
	"fmt"
	"os"
	"reflect"
	"regexp"
	"strings"
	"sync"
)

////////////////////////////////////////////////////////////////////////////////
// Helpers /////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// TestWriter - Writer implementation that will keep track of log lines
type TestWriter struct {
	mu      sync.Mutex
	entries *[]string
}

func (w TestWriter) Write(p []byte) (int, error) {
	w.mu.Lock()
	n, err := os.Stderr.Write(p)
	*w.entries = append(*(w.entries), string(p))
	w.mu.Unlock()
	return n, err
}

// ConfigStdLogWriter - Helper to configure test writer to capture Std log lines
func ConfigStdLogWriter(entries *[]string) {
	SetWriter(TestWriter{entries: entries})
	UseStdLogFormatter()
}

// ConfigJSONLogWriter - Helper to configure test writer to capture json log
// lines
func ConfigJSONLogWriter(entries *[]string) {
	SetWriter(TestWriter{entries: entries})
	UseJSONLogFormatter()
}

// ExpEntry - Helper struct to represent an expected log line
type ExpEntry struct {
	channel     string
	level       string
	hasGid      bool
	body        string
	nIndent     int
	servicename *string
	mapData     map[string]interface{}
}

func matchExp(entry string, exp ExpEntry, verbose bool) bool {

	// Big nasty regex to parse out the parts of a Std formatted log:
	//
	// Example log line:
	// 2017/04/14 19:32:15 <test_service> [SRVUT:INFO:1]     Serving insecure gRPC on port 54321
	//
	// - "^[0-9/]* [0-9:]*" - Parses the timestamp at the beginning of the line
	// - " ([^\\]]*)" - Parses any content after the timestamp, but before the
	//  bracked header. The only thing that can fall in here is the service name.
	//  This section is optional, so may be empty
	// - "\\[([^:]*):" - Open the bracketed header and parse the channel
	// - "([^\\]:]*)" - Parse the level
	// - "([^\\]\\s]*)\\]" - Parse the thread id if present (optional)
	// - " ([\\s]*)" - Parse the indentation whitespace
	// - "([^\\s].*)\n$" - Parse the message to the end of the line
	r := regexp.MustCompile("^[0-9/]* [0-9:]* ([^\\]]*)\\[([^:]*):([^\\]:]*)([^\\]\\s]*)\\] ([\\s]*)([^\\s].*)\n$")

	// Parse the log with the regex and make sure there's a (possibly empty) match
	// for each of the regex groups.
	m := r.FindStringSubmatch(entry)
	match := true
	if len(m) != 7 {
		if verbose {
			fmt.Printf("Failed to parse log line [%s]\n", entry)
		}
		match = false
	} else {
		if len(m[1]) > 0 && nil == exp.servicename {
			if verbose {
				fmt.Printf("Got unexpected service name string [%s]\n", m[1])
			}
			match = false
		} else if len(m[1]) == 0 && nil != exp.servicename {
			if verbose {
				fmt.Printf("Missing expected service name [%s]\n", *exp.servicename)
			}
			match = false
		} else if len(m[1]) > 0 {
			// The service name will be enclosed in angle-brackets if present, so find
			// the actual service name by stripping those off
			snRexp := regexp.MustCompile("<([^>]*)> ")
			snMatch := snRexp.FindStringSubmatch(m[1])
			if len(snMatch) != 2 {
				if verbose {
					fmt.Println("Missing service name")
				}
				match = false
			} else if snMatch[1] != *exp.servicename {
				if verbose {
					fmt.Printf("Service name mismatch. Got [%s], Expected [%s]\n", snMatch[1], *exp.servicename)
				}
				match = false
			}
		}
		if m[2] != exp.channel {
			if verbose {
				fmt.Printf("Channel string mismatch. Expected [%s], Got [%s]\n", exp.channel, m[1])
			}
			match = false
		}
		if m[3] != exp.level {
			if verbose {
				fmt.Printf("Level string mismatch. Expected [%s], Got [%s]\n", exp.level, m[2])
			}
			match = false
		}
		if len(m[4]) > 0 && !exp.hasGid {
			if verbose {
				fmt.Println("Got unexpected GID")
			}
			match = false
		}
		if len(m[4]) == 0 && exp.hasGid {
			if verbose {
				fmt.Println("Missing expected GID")
			}
			match = false
		}
		if m[5] != strings.Repeat(GetIndentString(), exp.nIndent) {
			if verbose {
				fmt.Printf("Indent mismatch. Expected [%s], Got [%s]\n", m[4], strings.Repeat(GetIndentString(), exp.nIndent))
				match = false
			}
		}
		if m[6] != exp.body {
			if verbose {
				fmt.Printf("Body string mismatch. Expected [%s], Got [%s]\n", exp.body, m[4])
			}
			match = false
		}
	}
	return match
}

// VerifyLogs - Verify that the expected STD log lines were logged
func VerifyLogs(entries []string, expected []ExpEntry) bool {
	if len(entries) != len(expected) {
		fmt.Printf("Length mismatch: Expected %d, Got %d\n", len(expected), len(entries))
		return false
	}
	match := true
	for i, entry := range entries {
		if !matchExp(entry, expected[i], true) {
			fmt.Printf("Match failed for entry [%d]\n", i)
			match = false
		}
	}
	return match
}

// VerifyLogsUnordered - Verify that the expecte log lines were logged,
// regardless of order
func VerifyLogsUnordered(entries []string, expected []ExpEntry) bool {
	if len(entries) != len(expected) {
		fmt.Printf("Length mismatch: Expected %d, Got %d\n", len(expected), len(entries))
		return false
	}
	match := true
	for _, exp := range expected {
		foundMatch := false
		for _, entry := range entries {
			if matchExp(entry, exp, false) {
				foundMatch = true
				break
			}
		}
		if !foundMatch {
			fmt.Printf("No match found for expected entry [%v]\n", exp)
			match = false
		}
	}
	return match
}

// VerifyJSONLogs - Verify that the expected JSON log lines were logged
func VerifyJSONLogs(entries []string, expected []ExpEntry) bool {
	if len(entries) != len(expected) {
		fmt.Printf("Length mismatch: Expected %d, Got %d\n", len(expected), len(entries))
		return false
	}
	match := true
	for i, entry := range entries {
		if !matchExpJSON(entry, expected[i]) {
			fmt.Printf("Match failed for entry [%d]\n", i)
			match = false
		}
	}
	return match
}

func matchExpJSON(entry string, expected ExpEntry) bool {

	// Parse to a LogEntry
	var logEntry LogEntry
	if le, err := JSONToLogEntry(entry); nil != err {
		fmt.Printf("Failed to unmarshal entry [%s]: %v\n", entry, err.Error())
		return false
	} else if nil == le {
		fmt.Printf("Failed to parse LogEntry [%s]\n", entry)
		return false
	} else {
		logEntry = *le
	}

	// Check fields
	match := true
	if string(logEntry.Channel) != expected.channel {
		fmt.Printf("Channel string mismatch. Got [%s], expected [%s]\n", string(logEntry.Channel), expected.channel)
		match = false
	}
	if LevelToHumanString(logEntry.Level) != expected.level {
		fmt.Printf("Level string mismatch. Got [%s], expected [%s]\n", LevelToHumanString(logEntry.Level), expected.level)
		match = false
	}
	if logEntry.Format != expected.body {
		fmt.Printf("Message mismatch. Got [%s], expected [%s]\n", logEntry.Format, expected.body)
		match = false
	}
	if logEntry.NIndent != expected.nIndent {
		fmt.Printf("Indent mismatch. Got [%d], expected [%d]\n", logEntry.NIndent, expected.nIndent)
		match = false
	}

	// Optional GID
	if expected.hasGid && nil == logEntry.GoroutineID {
		fmt.Printf("Missing expected GID\n")
		match = false
	} else if !expected.hasGid && nil != logEntry.GoroutineID {
		fmt.Printf("Got GID when none expected\n")
		match = false
	}

	// Optional service name
	if nil == expected.servicename && len(logEntry.Servicename) != 0 {
		fmt.Printf("Got unexpected service name [%s]", logEntry.Servicename)
		match = false
	} else if nil != expected.servicename && *expected.servicename != logEntry.Servicename {
		fmt.Printf("Service name mismatch. Got [%s], expected [%s]\n", logEntry.Servicename, *expected.servicename)
		match = false
	}

	// Map data
	if len(expected.mapData) != len(logEntry.MapData) {
		fmt.Printf("Mismatched mapData length. Got [%d], expected [%d]\n", len(logEntry.MapData), len(expected.mapData))
		match = false
	}
	for k, v := range expected.mapData {
		if gotVal, ok := logEntry.MapData[k]; !ok {
			fmt.Printf("Missing expected mapData entry [%s]\n", k)
			match = false
		} else if gotNumVal, ok := gotVal.(json.Number); ok {
			if gotNumVal.String() != fmt.Sprintf("%v", v) {
				fmt.Printf("Value mismatch for numerical mapData entry [%s]. Got: %v, expected %v\n", k, gotVal, v)
				match = false
			}
		} else if !reflect.DeepEqual(v, gotVal) {
			fmt.Printf("Value mismatch for mapData entry [%s]. Got: [%v - %s], expected [%v - %s]\n", k, gotVal, reflect.TypeOf(gotVal), v, reflect.TypeOf(v))
			match = false
		}
	}

	return match
}

// ValidateChannelMap - Compare an expected channel map to a configured map
func ValidateChannelMap(got, expected ChannelMap) bool {
	ch := UseChannel("TEST")
	res := true
	if len(got) != len(expected) {
		ch.Log(ERROR, "Channel map length mismatch. Got %d, Expected %d", len(got), len(expected))
		res = false
	}
	for k, expVal := range expected {
		if gotVal, ok := got[k]; !ok {
			ch.Log(ERROR, "Missing expected key [%s]", k)
			res = false
		} else if gotVal != expVal {
			ch.Log(ERROR, "Incorrect level for [%s]. Got [%s], Expected [%s]", k,
				LevelToHumanString(gotVal), LevelToHumanString(expVal))
			res = false
		}
	}
	return res
}
