# Alchemy Log (alog)
The `alog` package provides a wrapper layer on top of the standard `log` package
that allows for verbose logging at develop/debug time and minimal logging at
production run time.

## Channels and Levels
The primary components of the system are **channels** and **levels** which allow for
each log statement to be enabled or disabled when appropriate.

1. **Channels**: Each logging statement is made to a specific channel. Channels
are independent of one another and allow for logical grouping of log messages by
functionality. A channel can be any string.

1. **Levels**: Each logging statement is made at a specific level. Levels provide
sequential granularity, allowing detailed debugging statements to be placed in
the code without clogging up the logs at runtime. The sequence of levels and
their general usage is as follows:

  1. `off`: Disable the given channel completely
  1. `fatal`: A fatal error has occurred. Any behavior after this statement
  should be regarded as undefined.
  1. `error`: An unrecoverable error has occurred. Any behavior after this
  statement should be regarded as undefined unless the error is explicitly
  handled.
  1. `warning`: A recoverable error condition has come up that the service
  maintainer should be aware of.
  1. `info`: High-level information that is valuable at runtime under moderate
  load.
  1. `trace`: Used to log begin/end of functions for debugging code paths.
  1. `debug`: High-level debugging statements such as function parameters.
  1. `debug1`: High-level debugging statements.
  1. `debug2`: Mid-level debugging statements such as computed values.
  1. `debug3`: Low-level debugging statements such as computed values inside
  loops.
  1. `debug4`: Ultra-low-level debugging statements such as data dumps and/or
  statements inside multiple nested loops.

Using this combination of **Channels** and **Levels**, you can fine-tune what log
statements are enabled when you run your application under different
circumstances.

## Standard Configuration
There are two primary pieces of configuration when setting up the `alog`
environment:

1. **default_log_level**: This is the level that will be enabled for a given
channel when a specific level has not been set in the **log_filters**.

1. **log_filters**: This is a mapping from channel name to level that allows
levels to be set on a per-channel basis.

The `alog.Config()` function allows both the default level and filters to be set
at once. For example:

```go
import (
  "github.ibm.com/watson-vision/goutils/alog"
)

func foo() {
  alog.Config(alog.INFO, alog.ChannelMap{
    "FOO": alog.DEBUG,
    "BAR": alog.OFF,
  })
}
```

In this example, the channel `"FOO"` is set to the `DEBUG` level, the channel
`"BAR"` is fully disabled, and all other channels are set to use the `INFO`
level.

## Logging Functions
The standard logging functions each take a channel, a level, a format string,
and optional format values. Each one is a wrapper around the standard logging
functions from the `log` package. The functions are:

1. `Log`: Alias to `Printf`. This is the standard logging function.

1. `Printf`: Perform a standard logging statement.

1. `Panicf`: Perform a panic logging statement.

1. `Fatalf`: Perform a fatal logging statement.

Here's a simple example of a basic log statement:

```go
import (
  "github.ibm.com/watson-vision/goutils/alog"
)

func foo(age int) {
  alog.Log("DEMO", alog.INFO, "Hello Logging World! I am %d years old", age)
}
```

## Channel Log
In a given portion of code, it often makes sense to have a common channel that
is used by many logging statements. Re-typing the channel name can be cumbersome
and error-prone, so the concept of the **Channel Log** helps to eliminate this
issue. To create a Channel Log, call the `UseChannel` function. This gives you
a handle to a channel log which has all of the same standard log functions as
the top-level `alog`, but without the requirement to specify a channel. For
example:

```go
import (
  "github.ibm.com/watson-vision/goutils/alog"
)

var ch = alog.UseChannel("DEMO")

func foo(age int) {
  ch.Log(alog.INFO, "Hello Logging World! I am %d years old", age)
}
```

## LogScope and FnLog
One of the most common uses for logging is to note when a certain block of code
starts and ends. To facilitate this, `alog` has the concept of the `LogScope`.
A `LogScope` is a simple object which logs a `"Start:"` statement at creation
time and a `"End:"` statement at `Close()` time. All logging statements which
occur between creation and close will be indented, making for a highly readable
log, even with very verbose logging. Here's a simple example of `LogScope`:

```go
import (
  "github.ibm.com/watson-vision/goutils/alog"
)

var ch = alog.UseChannel("DEMO")

func foo(age int) {
  ch.Log(alog.INFO, "Hello Logging World! I am %d years old", age)
  {
    scope := ch.LogScope(alog.DEBUG, "This is my own personal scope bubble")
    ch.Log(alog.DEBUG, "Hey! I'm walking here!")
    scope.Close()
  }
}
```

The most common use of `LogScope` is to log the begin and end of a function. To
help with this, `alog` provides the `FnLog` and the `DetailFnLog` functions.
These are wrappers around `LogScope` which determine the name of the function
being called and add it to the log statement automatically. `FnLog` always logs
to the `trace` level, while `DetailFnLog` takes an explicit level. For example:

```go
import (
  "github.ibm.com/watson-vision/goutils/alog"
)

var ch = alog.UseChannel("DEMO")

func foo(age int) {
  defer ch.FnLog("%d", age).Close()
  do_foo()
}

func do_foo() {
  defer ch.DetailFnLog(alog.DEBUG, "").Close()
  ch.Log(alog.DEBUG, "Down in the weeds")
}
```

**WARNING** If you do not invoke `Close()` on your scope, your application will
have a memory leak. The `alog` config object holds a map from goroutine ID to
indentation level which is incremented at construct time and decremented at
close time. Once back to 0, the map entry is removed. If `Close()` is not
invoked, this map will grow indefinitely.

## Convenience Functions
There are several other convenience functions available with the `alog` package:

1. `Indent`/`Deindent`: These functions can be used to manually manage
indentation within blocks of code. Note that they carry the same **WARNING** as
`LogScope` in that an equal number of `Deindent` calls must be made to match the
`Indent` calls or a memory leak will ensue.

1. `LevelToHumanString`: This will convert a log level to a human readable
string that will match the string used for configuration input.

1. `PrintConfig`: This constructs a string representation of the current default
level and channel map.

1. `GetDefaultLevel`: Get the default level that's currently configured.

1. `GetChannelMap`: Get the currently configured channel map.

1. `IsEnabled`: This basic predicate allows for the construction of labor-
intensive log statements to be wrapped in an if block and only executed when the
given channel/level pair is active. It should NOT be used to wrap functional
code since that code would be disabled if the check fails. Here's an example of
correct usage:

```go
import (
  "fmt"
  "github.ibm.com/watson-vision/goutils/alog"
)

var ch = alog.UseChannel("DEMO")

func foo(ages map[string]int) {
  defer ch.FnLog("%d", age).Close()
  if ch.IsEnabled(alog.INFO) {
    s := "Age Map:\n"
    for name, age := range ages {
      s += fmt.Sprintf("  %s: %d\n", name, age)
    }
    ch.Log(alog.INFO, s)
  }
}
```

## Advanced Configuration
In addition to the standard configuration for default level and filters, there
are a number of additional configuration functions:

1. `ResetDefaults`: Reset configuration to all standard defaults.

1. `ConfigWriter`: Set the `io.Writer` instance to use as the backend for
logging. This can be used to send log statements to places other than
`os.Stderr`. It directly overwrites the `io.Writer` used by the standard `log`
instance.

1. `ConfigPrefix`: Set the string prefix that the underlying `log` instance will
use.

1. `ConfigFlags`: Set the flags that the underlying `log` instance will use.

1. `SetMaxChannelLen`: Set the truncation length for channel strings in the
header.

1. `EnableGID`/`DisableGID`: These functions enable or disable printing the
numeric goroutine ID as part of the log statement header.

1. `EnableFullFuncSig`/`DisableFullFuncSig`: These functions enable or
disable printing the full function signature as part of the `FnLog` functions.

# Alog Extras
In addition to the core functionality, a number of convenient extras come along
with the `alog` package to help with common usage patterns.

## Command Line Configuration
The most common usage for `alog` is as a command-line configurable logging
framework. As such, a standard set of command line flags are provided with
documentation. The important functions for this functionality are:

1. `GetFlags`: Construct the standard set of `alog` command line flags.

1. `ConfigureFromFlags`: Configure `alog` using the parsed command line flags.

Here's a simple example:

```go
package main

import (
  "flag"
  "github.ibm.com/watson-vision/goutils/alog"
)

var ch = alog.UseChannel("MAIN")

func main() {

  // Set up logging from command line
  logFlags := alog.GetFlags()
  flag.Parse()
  alog.ConfigureFromFlags(logFlags)

  // Do some logging
  ch.Log(alog.INFO, "Hello World!")
  ch.Log(alog.DEBUG2, "lots of details...")
  alog.Log("OTHER", alog.INFO, "Logging on a different channel")
}
```

## Command Line Flags
Here's the overview of the available command-line options:

* `log.default-level`: Set the default log level using one of the strings
  defined in the [Levels Section](#channels-and-levels).
* `log.filters`: Filter string in the form `"CHAN1:info,CHAN2:debug"`.
* `log.chan-header-len`: Set the length of the channel string in the header.
* `log.goroutine-id`: Include a unique numeric ID for the goroutine in each
  header.
* `log.function-signature`: Log the fully-qualified function signature for
  `FnLog` invocations. If false, only the function name is logged.
* `log.no-indent`: Fully disable indentation
* `log.service-name`: Log a common label for the service with each entry
  (useful for publishing to common logging service).
* `log.output-json`: Output log entries as structured json rather than plain
  text.

## Dynamic HTTP Server Logging
When implementing an HTTP server, it can be very useful to allow for dynamic
logging so that the server can be launched with logging disabled, but have it
enabled for a short time to inspect traffic. To facilitate this, the
`DynamicHandler` can be bound to a route and called with the following
parameters:

1. `default_log_level`: String representing a new default log level to use.

1. `log_filters`: New channel map to use, formatted as a string with commas
separating entries and colons separating key/value pairs. For example
`"FOO:debug,BAR:info"`.

1. `timeout`: If provided, the changes will automatically be reverted in the
provided number of seconds.

Here's a simple example:

```go
package main

import (
  "flag"
  "github.ibm.com/watson-vision/goutils/alog"
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
  alog.ConfigureFromFlags(logFlags)
  ch.Log(alog.INFO, "Hello World!")

  // Bind dynamic log handler
  http.HandleFunc("/logging", alog.DynamicHandler)

  // Bind simple function that does some logging
  http.HandleFunc("/demo", func(w http.ResponseWriter, r *http.Request){
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
```
