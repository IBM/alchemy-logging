# Alchemy Logging (alog) - Python
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time.

## Setup
To use the `alog` module, simply install it with `pip`:

```sh
pip install alchemy-logging
```

## Channels and Levels
The primary components of the framework are **channels** and **levels** which allow for each log statement to be enabled or disabled when appropriate.

1. **Levels**: Each logging statement is made at a specific level. Levels provide sequential granularity, allowing detailed debugging statements to be placed in the code without clogging up the logs at runtime. The sequence of levels and their general usage is as follows:

    1. `off`: Disable the given channel completely
    1. `fatal`: A fatal error has occurred. Any behavior after this statement should be regarded as undefined.
    1. `error`: An unrecoverable error has occurred. Any behavior after this statement should be regarded as undefined unless the error is explicitly handled.
    1. `warning`: A recoverable error condition has come up that the service maintainer should be aware of.
    1. `info`: High-level information that is valuable at runtime under moderate load.
    1. `trace`: Used to log begin/end of functions for debugging code paths.
    1. `debug`: High-level debugging statements such as function parameters.
    1. `debug1`: High-level debugging statements.
    1. `debug2`: Mid-level debugging statements such as computed values.
    1. `debug3`: Low-level debugging statements such as computed values inside loops.
    1. `debug4`: Ultra-low-level debugging statements such as data dumps and/or statements inside multiple nested loops.

1. **Channels**: Each logging statement is made to a specific channel. Channels are independent of one another and allow for logical grouping of log messages by functionality. A channel can be any string. A channel may have a specific **level** assigned to it, or it may use the configured default level if it is not given a specific level filter.

Using this combination of **Channels** and **Levels**, you can fine-tune what log statements are enabled when you run your application under different circumstances.

## Usage

### Configuration

```py
import alog

if __name__ == "__main__":
    alog.configure(default_level="info", filters="FOO:debug,BAR:off")
```

In this example, the channel `"FOO"` is set to the `debug` level, the channel `"BAR"` is fully disabled, and all other channels are set to use the `INFO` level.

In addition to the above, the `configure` function also supports the following arguments:

* `formatter`: May be `"pretty"`, `"json"`, or any class derived from `AlogFormatterBase`
* `thread_id`: Bool indicating whether or not to include a unique thread ID with the logging header (`pretty`) or structure (`json`).
* `handler_generator`: This allows users to provide their own output handlers and replace the standard handler that sends log messages to `stderr`. See [the `logging` documentation](https://docs.python.org/3/library/logging.handlers.html#module-logging.handlers) for details.

### Logging Functions
For each log level, there are two functions you can use to create log lines: The standard `logging` package function, or the corresponding `alog.use_channel(...).<level>` function. The former will always log to the `root` channel while the later requires that
a channel string be specified via `use_channel()`.

```py
import alog
import logging

def foo(age):
    alog.use_channel("FOO").debug3(
        "Debug3 line on the FOO channel with an int value %d!", age
    )
    logging.debug("debug line on the MAIN channel")
```

### Channel Log
In a given portion of code, it often makes sense to have a common channel that is used by many logging statements. Re-typing the channel name can be cumbersome and error-prone, so the concept of the **Channel Log** helps to eliminate this issue. To create a Channel Log, call the `use_channel` function. This gives you a handle to a channel log which has all of the same standard log functions as the top-level `alog`, but without the requirement to specify a channel. For example:

```py
import alog

log = alog.use_channel("FOO")

def foo(age):
    log.info("Hello Logging World! I am %d years old", age)
```

**NOTE**: In this (python) implementation, this is simply a wrapper around `logging.getLogger()`

### Extra Log Information

There are several other types of information that `alog` supports adding to log records:

#### Log Codes

This is an optional argument to all logging functions which adds a specified code to the record. It can be useful for particularly high-profile messages (such as per-request error summaries in a server) that you want to be able to track in a programmatic way. The only requirement for a `log_code` is that it begin with `<` and end with `>`. The log code always comes before the `message`. For example:

```py
ch = alog.use_channel("FOO")
ch.debug("<FOO80349757I>", "Logging is fun!")
```

#### Dict Data

Sometimes, it's useful to log structured key/value pairs in a record, rather than a plain-text message, even when using the `pretty` output formatter. To do this, simply use a `dict` in place of a `str` in the `message` argument to the logging function. For example:

```py
ch = alog.use_channel("FOO")
ch.debug({"foo": "bar"})
```

When a `dict` is logged with the `json` formatter enabled, all key/value pairs are added as key/value pairs under the top-level `message` key.

### Log Contexts
One of the most common uses for logging is to note events when a certain block of code executes. To facilitate this, `alog` has the concept of log contexts. The two primary contexts that `alog` supports are:

* `ContextLog`: This [contextmanager](https://docs.python.org/3/library/contextlib.html#contextlib.contextmanager) logs a `START:` message when the context starts and an `END:` message when the context ends. All messages produced within the same thread inside of the context will have an incremented level of indentation.

```py
import alog

alog.configure("debug2")
log = alog.use_channel("DEMO")

with alog.ContextLog(log.info, "Doing some work"):
    log.debug("Deep in the muck!")
```

```
2021-07-29T19:09:03.819422 [DEMO :INFO] BEGIN: Doing some work
2021-07-29T19:09:03.820079 [DEMO :DBUG]   Deep in the muck!
2021-07-29T19:09:03.820178 [DEMO :INFO] END: Doing some work
```

* `ContextTimer`: This [contextmanager](https://docs.python.org/3/library/contextlib.html#contextlib.contextmanager) starts a timer when the context begins and logs a message with the duration when the context ends.

```py
import alog
import time

alog.configure("debug2")
log = alog.use_channel("DEMO")

with alog.ContextTimer(log.info, "Slow work finished in: "):
    log.debug("Starting the slow work")
    time.sleep(1)
```

```
2021-07-29T19:12:00.887949 [DEMO :DBUG] Starting the slow work
2021-07-29T19:12:01.890839 [DEMO :INFO] Slow work finished in: 0:00:01.002793
```

### Function Decorators
In addition to arbitrary blocks of code that you may wish to scope or time, a very common use case for logging contexts is to provide function tracing. To this end, `alog` provides two useful function decorators:

* `@logged_function`: This [decorator](https://www.python.org/dev/peps/pep-0318/) wraps the `ContextLog` and provides a `START`/`END` scope where the message is prepopulated with the name of the function.

```py
import alog

alog.configure("debug")
log = alog.use_channel("DEMO")

@alog.logged_function(log.trace)
def foo():
    log.debug("This is a test")

foo()
```

```
2021-07-29T19:16:40.036119 [DEMO :TRCE] BEGIN: foo()
2021-07-29T19:16:40.036807 [DEMO :DBUG]   This is a test
2021-07-29T19:16:40.036915 [DEMO :TRCE] END: foo()
```

* `@timed_function`: This [decorator](https://www.python.org/dev/peps/pep-0318/) wraps the `ContextTimer` and performs a scoped timer on the entire function.

```py
import alog
import time

alog.configure("debug")
log = alog.use_channel("DEMO")

@alog.timed_function(log.trace)
def foo():
    log.debug("This is a test")
    time.sleep(1)

foo()
```

```
2021-07-29T19:19:47.468428 [DEMO :DBUG] This is a test
2021-07-29T19:19:48.471788 [DEMO :TRCE] 0:00:01.003284
```

## Tip
- Visual Studio Code (VSCode) users can take advantage of [alchemy-logging extension](https://marketplace.visualstudio.com/items?itemName=Gaurav-Kumbhat.alog-code-generator) that provides automatic log code generation and insertion.
