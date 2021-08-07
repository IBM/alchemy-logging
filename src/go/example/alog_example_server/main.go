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
