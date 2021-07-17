# Alchemy Logging (alog) - Go
The `go` (`golang`) implementation of `alog` provides all of the core features of the framework in a go-native implementaiton designed to look and feel like the standard [`log`](https://pkg.go.dev/log) package.

## Configuration

The `alog.Config()` function allows both the default level and filters to be set at once. For example:

```go
import (
  "github.ibm.com/watson-discovery/alog"
)

func foo() {
  alog.Config(alog.INFO, alog.ChannelMap{
    "FOO": alog.DEBUG,
    "BAR": alog.OFF,
  })
}
```
