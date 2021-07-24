# Alchemy Logging (alog) - Python
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time.

## Setup
To use the `alog` module, simply install it with `pip`:

```sh
pip install alchemy-logging
```

## Channels and Levels
The primary components of the system are **channels** and **levels** which allow for each log statement to be enabled or disabled when appropriate.

1. **Channels**: Each logging statement is made to a specific channel. Channels are independent of one another and allow for logical grouping of log messages by functionality. A channel can be any string.

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

**NOTE**: The level string `'disable'` can be used to fully disable *all* logging. This is not a conventional `level` in the sense that it is not providing a sequential comparison that each message is checked against. Instead, it fully disables all logging using the underlying `logging` framework.

Using this combination of **Channels** and **Levels**, you can fine-tune what log statements are enabled when you run your application under different circumstances.

## Standard Configuration
There are two primary pieces of configuration when setting up the `alog` environment:

1. **default_level**: This is the level that will be enabled for a given channel when a specific level has not been set in the **filters**.

1. **filters**: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

The `alog.Config()` function allows both the default level and filters to be set at once. For example:

```py
import alog

if __name__ == '__main__':
  alog.configure(default_level='info', filters='FOO:debug,BAR:off')
```

In this example, the channel `"FOO"` is set to the `debug` level, the channel `"BAR"` is fully disabled, and all other channels are set to use the `INFO` level.

## Output Formatting
There are two default output formats supported by `alog`: Pretty and json. To configure the output format, use the `formatter` argument to `alog.configure`. The valid options are:

* `pretty`: Easily visible logging for quick visual parsing
* `json`: Formatted logging for systemic log data creation

Since `alog` is simply a wrapper around the standard python `logging` package, you can also add your own formatters using the standard mechanisms provided in that package.

## Logging Functions
For each log level, there are two functions you can use to create log lines: The standard `logging` package function (with additional functions for higher debug levels), or the corresponding `alog.<level>` function. The former will always log to the `MAIN` channel while the later requires that
a channel string be specified.

```py
import alog
import logging

def foo():
  alog.use_channel('FOO').debug('Debug line on the FOO channel with an int value %d!', 10)
  logging.debug3('debug3 line on the MAIN channel')
```

## Channel Log
In a given portion of code, it often makes sense to have a common channel that is used by many logging statements. Re-typing the channel name can be cumbersome and error-prone, so the concept of the **Channel Log** helps to eliminate this issue. To create a Channel Log, call the `use_channel` function. This gives you a handle to a channel log which has all of the same standard log functions as the top-level `alog`, but without the requirement to specify a channel. For example:

```py
import alog

ch = alog.use_channel("FOO")

def foo():
  ch.info("Hello Logging World! I am %d years old", age)
```

**NOTE**: In this (python) implementation, this is simply a wrapper around `logging.getLogger()`

## LogScope and FnLog
One of the most common uses for logging is to note when a certain block of code starts and ends. To facilitate this, `alog` has the concept of the `LogScope`. A `LogScope` is a simple object which logs a `"START:"` statement at creation time and a `"END:"` statement at destruction time. All logging statements which occur between creation and close will be indented, making for a highly readable log, even with very verbose logging. Here's a simple example of `LogScope`:

```py
import alog

ch = alog.use_channel("DEMO")

def foo(age):
  ch.info("Hello Logging World! I am %d years old", age)
  if True:
    scope = alog.LogScope(ch.debug, "This is my own personal scope bubble")
    ch.debug("Hey! I'm walking here!")
    del scope
```

**NOTE**: Since scoping is permeable in python, if you create nested `LogScope` instances, you must explicitly delete them when the scope closes.

The most common use of `LogScope` is to log the begin and end of a function. To help with this, `alog` provides the `FnLog` object. This is a wrapper around `LogScope` which determines the name of the function being called and adds it to the log statements automatically. For example:

```py
import alog

ch = alog.use_channel("DEMO")

def foo(age):
  _ = alog.FnLog(ch.trace, "%d", age)
  do_foo()

def do_foo():
  _ = alog.FnLog(ch.debug)
  ch.debug("Down in the weeds")
```

## Unit testing

Currently the unit tests live in `tests/unit_tests.py`. You can run them with the following:

```sh
# One-time setup to get the development environmet set up
python setup.py develop

# Run the tests
./ci/run-tests.sh
```

Note: A few tests still live in `alog/alog.py` that should be migrated over to the unit tests directory as time permits.
