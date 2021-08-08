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

package main

import (
	"flag"
	"github.com/IBM/alchemy-logging/src/go/alog"
	"net/http"
	"strings"
)

var ch = alog.UseChannel("MAIN")

func main() {

	// Get a port from the command line
	listenPort := flag.String(
		"port",
		"54321",
		"port bound to this service")

	// Set up logging from command line
	logFlags := alog.GetFlags()
	flag.Parse()
	if err := alog.ConfigureFromFlags(logFlags); nil != err {
		alog.Fatalf("MAIN", alog.FATAL, err.Error())
	}
	ch.Log(alog.INFO, "Hello World!")

	// Bind dynamic log handler
	http.HandleFunc("/logging", alog.DynamicHandler)

	// Bind simple function that does some logging
	http.HandleFunc("/demo", func(w http.ResponseWriter, r *http.Request) {
		ch := alog.UseChannel("HNDLR")
		defer ch.LogScope(alog.TRACE, "Handling /demo").Close()

		ch.Log(alog.WARNING, "WATCH OUT!")
		ch.Log(alog.INFO, "Standard stuff...")
		if ch.IsEnabled(alog.DEBUG) {
			ch.Log(alog.DEBUG, "Query Params:")
			r.ParseForm()
			for k, vals := range r.Form {
				ch.Log(alog.DEBUG, "  * %s: %s", k, strings.Join(vals, ", "))
			}
		}
		w.WriteHeader(http.StatusOK)
	})

	// Start serving requests
	ch.Log(alog.FATAL, "%s", http.ListenAndServe(":"+*listenPort, nil))
}
