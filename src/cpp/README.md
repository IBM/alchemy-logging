# Alchemy Logging (alog) - c++
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time.

## Dependencies
The `alog` library is intentionally light on dependencies. The only dependencies necessary are [nlohmann/json](hhttps://github.com/nlohmann/json) and [boost/algorithm](https://github.com/boostorg/algorithm/tree/boost-1.69.0) and [boost/locale](https://github.com/boostorg/locale/tree/boost-1.69.0).

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

## Configuration
There are two primary pieces of configuration when setting up the `alog` environment:

1. **default_level**: This is the level that will be enabled for a given channel when a specific level has not been set in the **filters**.

1. **filters**: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

The `ALOG_SETUP(...)` function allows both the default level and filters to be set at once. For example:

```c++
#include <alog/logger.hpp>

int main()
{
  ALOG_SETUP("info", "FOO:debug2,BAR:off");
}
```

## Structured Logging
As `alog` has grown, its use has tended towards usage as part of a multi-replica cluster of servers. In such an environment, it can be very beneficial to provide structure in your log messages so that they can be aggregated between replicas and used for operational visibility. The simplest way to do this is to log lines as `json` rather than the traditional pretty-print formatting. By default `alog` uses the pretty-printer output formatter. To enable JSON output, simply use the `ALOG_USE_JSON_FORMATTER()` macro.

```c++
#include <alog/logger.hpp>

int main()
{
  ALOG_SETUP("info", "FOO:debug2,BAR:off");
  ALOG_USE_JSON_FORMATTER();
}
```

While printing logs as `json` allows them to be filtered by `channel`, `level`, and `message` quite easily, some times more structure is needed. In these cases, `alog` also supports logging arbitrary key/value pairs. This is supported using the `nlohmann/json` map type. For example:

```c++
#include <alog/logger.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{
  ALOG_SETUP("info", "FOO:debug2,BAR:off");
  ALOG_USE_JSON_FORMATTER();
  json mapData;
  mapData["foo"] = "bar";
  mapData["baz"] = 1234;
  ALOG_MAP(MAIN, info, mapData);
}
```

Also, all of the standard `ALOG` macros take an optional final `map` argument, allowing key/value data to be added to any log line.

## Metadata
In addition to the content of an individual log message, you may want to attach some metadata to all log lines that occur within a given thread of execution. For example, this can be used to attach a request ID to all log lines created in the course of processing a given server request. This can come in very handy when you have a multi-threaded and/or multi-replica environment.

The `metadata` feature in `alog` is implemented as a thread-global key/value map. Keys and values are added to the map using the `ALOG_SCOPED_METADATA` macro. We explicitly limit metadata interactions to construct/destruct scope object to ensure that metadata is properly cleaned up.

The addition of metadata can be expensive and intrusive (especially in pretty-print mode), so the functionality can be globally enabled or disabled using the `ALOG_ENABLE_METADATA` and `ALOG_DISABLE_METADATA` macros. These should be called as part of the initial configuration.

Here's a brief example of how you might use metadata:

```c++
#include <alog/logger.hpp>

int add(int a, int b)
{
  ALOG(MATH, info, "Adding " << a << " + " << b);
  return a+b;
}

void handler(const CMyRequest& a_request, CMyResponse& a_response)
{
  ALOG_SCOPED_METADATA("request_id", a_request.requestID());
  a_response.setResult(add(a_request.getA(), a_request.getB()));
}
```

In this example, the `add` function doesn't need (or want) to know that it's part of handling some sort of `MyRequest`, but the developer would like to attach the `requestID()` to the log line printed in its implementation in case there's a bug.

## Logging Macros
The standard logging macros each take a channel, a level, and payload information:

* `ALOG(channel, level, msg, [map])`: Log a single message line using streams
    ```c++
    ALOG(MAIN, debug, "This is the " << 1 << "st test with a stream");
    ```

* `ALOG_THREAD(channel, level, msg, [map])`: Log a single message line with the thread ID added to the header
    ```c++
    ALOG_THREAD(MAIN, debug, "This is the " << 2 << "nd test with a stream");
    ```

* `ALOG_MAP(channel, level, map)`: Log an arbitrary key/value structure on the given channel/level
    ```c++
    ALOG_MAP(MAIN, debug, (json{{"foo": "bar"}, {"baz", 1}}));
    ```

* `ALOG_WARNING(msg)`: Log a warning message on the `WARN` channel at the `warning` level.
    ```c++
    ALOG_WARNING("Something's wrong!");
    ```

## Log Scopes
One of the most common uses for logging is to note when a certain block of code starts and ends. To facilitate this, `alog` has the concept of the `ALOG_SCOPED_*` macros. Each of these macros creates a local object which takes an action at construction time and a corresponding action at destruction time. All logging statements which occur between construction and destruction will be indented, making for a highly readable log, even with very verbose logging.

Each of the scopes that creates log lines can take an optional `mapPtr` final argument. This allows for the scope to hold a pointer to a shared map which can have key/value pairs added within the context of the scope. This is particularly useful for `ALOG_SCOPED_TIMER`, but can be used elsewhere as well.

* `ALOG_SCOPED_BLOCK(channel, level, msg, [mapPtr])`: Set up a Start/End block of logging based on the scope. Note that only a single call to ALOG_SCOPED_BLOCK may be made within a given scope.
    ```c++
    void foo()
    {
      if (bar())
      {
        ALOG_SCOPED_BLOCK(MAIN, debug, "Bar is true!");
      }
    }
    ```

* `ALOG_SCOPED_TIMER(channel, level, msg, [mapPtr])`: Set up a timer that will time the work done in the current scope and report the duration upon scope completion.
    ```c++
    void foo()
    {
      if (bar())
      {
        std::shared_ptr<json> map_ptr(new json());
        ALOG_SCOPED_TIMER(MAIN, debug, "heavy_lifting", map_ptr);
        int result = heavy_lifting();
        (*map_ptr)["result_code"] = result;
      }
    }
    ```

* `ALOG_NEW_SCOPED_TIMER(channel, level, msg, [mapPtr])`: Create a new timer instance that will time the work done in the current scope and report the duration upon scope completion. This must be assigned to a named variable and can then be used to query for the current duration directly.
    ```c++
    void voo()
    {
        if (bar())
        {
            const auto timer = ALOG_NEW_SCOPED_TIMER(MAIN, debug, "Done handling bar in: ");

            // Expensive operation
            baz();

            ALOG(MAIN, debug, "Duration after baz(): " << timer.getCurrentDurationNS() << "ns");

            // Expensive operation
            buz();
        }
    }
    ```

* `ALOG_FUNCTION(channel, msg, [mapPtr])`: Add a Start/End indented block with the current function name on trace and add an automatic level of indentation.
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

* `ALOG_DETAIL_FUNCTION(channel, level, msg, [mapPtr])`: Add a Start/End indented block with the current function name on the given level and add an automatic level of indentation.
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

* `ALOG_SCOPED_METADATA(key, value)`: Add a key/value pair that will be added to every subsequent line in the scope
    ```c++
    #include <logger.h>

    int add(int a, int b)
    {
      ALOG(MATH, info, "Adding " << a << " + " << b);
      return a+b;
    }

    void handler(const CMyRequest& a_request, CMyResponse& a_response)
    {
      ALOG_SCOPED_METADATA("request_id", a_request.requestID());
      a_response.setResult(add(a_request.getA(), a_request.getB()));
    }
    ```

* `ALOG_SCOPED_INDENT_IF(channel, level)`: Add a level of indentation to all logging lines within the current scope if the given channel/level combination is enabled.
    ```c++
    void foo()
    {
      if (bar())
      {
        ALOG_SCOPED_BLOCK(MAIN, debug, "Bar is true!");
        ALOG_SCOPED_INDENT_IF(MAIN, debug2);
        ALOG(MAIN, debug2, "Getting it done");
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
