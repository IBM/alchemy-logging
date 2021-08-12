# Alchemy Logging (alog)
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time, all while avoiding performance degredation by using lazily evaluated log messages which only render if enabled. The `alog` project maintains language-naitve implementations in many popular programming languages with the goal of giving a consistent application logging experience both when writing your code and when administering a cloud service written in multiple programming languages.

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

## Standard Configuration
There are two primary pieces of configuration when setting up the `alog` environment:

1. **default_level**: This is the level that will be enabled for a given channel when a specific level has not been set in the **filters**.

1. **filters**: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

## Formatting
All `alog` implementations support two styles of formatting:

1. `pretty`: The **pretty** formatter is designed for development-time logging, making it easy to visually scan your log messages and follow the flow of execution

1. `json`: The **json** formatter is designed for production-runtime logging, making it easy to ingest structured representations of your runtime logs into a cloud log monitoring framework such as [Sysdig](https://sysdig.com/)

As a convenience, a converter is also provided from `json` -> `pretty` so that production logs can be read and visually parsed in a similar manner to development-time logs. The `pretty` -> `json` conversion is not provided since the header formatting for `pretty` logs can be lossy when channel names are truncated.

## Implementations

This section provides the high level details on installing and using each available implementation.

### Python

For full details see [src/python](src/python/README.md)

#### Installation

```sh
pip install alchemy-logging
```

#### Usage

```py
import alog
alog.configure(default_level="info", filters="FOO:debug2,BAR:off")
channel = alog.use_channel("FOO")
channel.debug("Hello alog!")
```

### Typescript

For full details see [src/ts](src/ts/README.md)

#### Installation

```sh
npm install alchemy-logging
```

#### Usage

```ts
import * as alog from 'alchemy-logging';
alog.configure('info', 'FOO:debug2,BAR:off');
alog.debug('FOO', 'Hello alog!');
```

### Go

For full details see [src/go](src/go/README.md)

#### Installation

```sh
go get github.com/IBM/alchemy-logging/src/go/alog
```

#### Usage

```go
package main
import (
	"github.com/IBM/alchemy-logging/src/go/alog"
)
func main() {
	alog.Config(alog.INFO, alog.ChannelMap{
		"FOO": alog.DEBUG2,
		"BAR": alog.OFF,
	})
	alog.Log("FOO", alog.DEBUG, "Hello alog!")
}
```

### C++

For full details see [src/cpp](src/cpp/README.md)

#### Installation

In your `CMakeLists.txt`:

```cmake
include(cmake/CPM.cmake)
set(ALOG_VERSION main CACHE STRING "The version (point in git history) of alog to use")
CPMAddPackage(
  NAME alog
  GITHUB_REPOSITORY IBM/alchemy-logging
  GIT_TAG ${ALOG_VERSION}
  GIT_SHALLOW true
  OPTIONS
    "BUILD_UNIT_TESTS OFF"
)
```

#### Usage

```cpp
#include <alog/logger.hpp>

int main()
{
    ALOG_SETUP("info", "FOO:debug2,BAR:off");
    ALOG(FOO, debug2, "Hello alog!");
}
```
