# Alchemy Log (alog)
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time.

## Dependencies
The `alog` library is intentionally light on dependencies. The only dependency necessary is the [nlohmann/json](https://github.com/nlohmann/json) library. That library is itself intentionally light and is integrated as a header-only that is included [directly](include/nlohmann/json.hpp).

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

Using this combination of **Channels** and **Levels**, you can fine-tune what log statements are enabled when you run your application under different circumstances.

## Standard Configuration
There are two primary pieces of configuration when setting up the `alog` environment:

1. **default_level**: This is the level that will be enabled for a given channel when a specific level has not been set in the **filters**.

1. **filters**: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

The `ALOG_SETUP(...)` macro allows both the default level and filters to be set at once. For example:

```c++
#include <logger.h>

int main(int, char**)
{
  ALOG_SETUP("server.log", true, "info", "ALGO:debug2,NET:debug");
}
```

By default `alog` uses the pretty-printer output formatter. To enable JSON output, simply use the `ALOG_USE_JSON_FORMATTER()` macro.

```c++
#include <logger.h>

int main(int, char**)
{
  ALOG_SETUP("server.log", true, "info", "ALGO:debug2,NET:debug");
  ALOG_USE_JSON_FORMATTER();
}
```

## Logging Functions
The standard logging functions each take a channel, a level, and payload information:

* `ALOG(channel, level, msg)`: Log a single message line using streams
    ```c++
    ALOG(MAIN, debug, "This is the " << 1 << "st test with a stream");
    ```

* `ALOG_MAP(channel, level, map)`: Log an arbitrary key/value structure on the given channel/level
    ```c++
    ALOG_MAP(MAIN, debug, {{"foo": "bar"}, {"baz", 1}});
    ```

* `ALOG_THREAD(channel, level, msg)`: Log a single message line with the thread ID added to the header
    ```c++
    ALOG_THREAD(MAIN, debug, "This is the " << 2 << "nd test with a stream");
    ```

* `ALOG_WARNING(msg)`: Log a warning message on the `WARN` channel at the `warning` level.
    ```c++
    ALOG_WARNING("Something's wrong!");
    ```

In addition to the single-line logging functions, `alog` offers scoped logging blocks which help with log parsing by providing `Start:` and `End:` messages for logical scopes:

* `ALOG_SCOPED_BLOCK(channel, level, msg)`: Set up a Start/End block of logging based on the scope. Note that only a single call to ALOG_SCOPED_BLOCK may be made within a given scope.
    ```c++
    void foo()
    {
      if (bar())
      {
        ALOG_SCOPED_BLOCK(MAIN, debug, "Bar is true!");
      }
    }
    ```

* `ALOG_SCOPED_TIMER(channel, level, msg)`: Set up a timer that will time the work done in the current scope and report the duration upon scope completion.
    ```c++
    void foo()
    {
      if (bar())
      {
        ALOG_SCOPED_TIMER(MAIN, debug, "heavy_lifting");
        heavy_lifting();
      }
    }
    ```

* `ALOG_SCOPED_INDENT()`: Add a level of indentation to all logging lines within the current scope to improve readibility.
    ```c++
    void foo()
    {
      if (bar())
      {
        ALOG_SCOPED_BLOCK(MAIN, debug, "Bar is true!");
        ALOG_SCOPED_INDENT();
        ALOG(MAIN, debug2, "Getting it done");
      }
    }
    ```

* `ALOG_FUNCTION(channel, msg)`: Add a Start/End indented block with the current function name on trace and add an automatic level of indentation.
    ```c++
    void foo()
    {
      ALOG_FUNCTION(TEST, "testing");
      if (bar())
      {
        ALOG(MAIN, debug2, "Getting it done");
      }
    }
    ```

* `ALOG_DETAIL_FUNCTION(channel, level, msg)`: Add a Start/End indented block with the current function name on the given level and add an automatic level of indentation.
    ```c++
    void foo()
    {
      ALOG_DETAIL_FUNCTION(TEST, debug2, "in the weeds");
      if (bar())
      {
        ALOG(MAIN, debug2, "Getting it done");
      }
    }
    ```

## Use Channel (this)
For each of the above macros, there is a corresponding `this` macro which doesn't require a `channel`. In the spirit of `channels`, log entries should be grouped by logical function. In order to make this easier, `alog` provides the following two additional macros:

* `ALOG_USE_CHANNEL(channel)`: Define a member function that will be used with the XXXthis macros
    ```c++
    class Foo
    {
    public:
      ALOG_USE_CHANNEL(FOO);

      void doit() const
      {
        ALOG_FUNCTIONthis("");
        ALOGthis(debug2, "We're doing this!");
      }
    };
    ```

* `ALOG_USE_CHANNEL_FREE(channel)`: Define a free function that will be used with the XXXthis macros. **NOTE**: This should only be used in a main compilation unit as it will cause conflicts with clas-specific `ALOG_USE_CHANNEL` invocations.
    ```c++
    ALOG_USE_CHANNEL_FREE(MAIN);
    void foo()
    {
      ALOG_DETAIL_FUNCTIONthis(debug2, "in the weeds");
      if (bar())
      {
        ALOGthis(debug2, "Getting it done");
      }
    }
    ```
