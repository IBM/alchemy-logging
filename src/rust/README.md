# Alchemy Logging (alog) - Rust
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time.

## Dependencies
The `alog` crate is intentionally light on dependencies. The only dependencies are [`serde`](https://crates.io/crates/serde) and [`serde_json`](https://crates.io/crates/serde_json), used for the JSON formatter and for structured extra data attached to log records.

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

`alog` models this with two enums instead of C++'s single level type: [`Level`] includes `Off` and is used for configuration/filtering, while [`MessageLevel`] excludes `Off` and is what the logging macros accept. This means attempting to log a message "at" `Off` is a compile error rather than a silent no-op at runtime.

## Configuration
There are two primary pieces of configuration when setting up the `alog` environment:

1. **default_level**: This is the level that will be enabled for a given channel when a specific level has not been set in the **filters**.

1. **filters**: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

The `alog::configure(...)` function allows both the default level and filters to be set at once. For example:

```rust
use alog::{alog, Config, Filters, MessageLevel};

fn main() {
    alog::configure(Config {
        default_level: "info".parse().unwrap(),
        filters: Filters::Spec("FOO:debug2,BAR:off".to_string()),
        ..Default::default()
    });
    alog!("MAIN", MessageLevel::Info, "Hello world");
}
```

If you only want to adjust the level/filters after initial setup (leaving the formatter and sink untouched), use `alog::adjust_levels(default_level, filters)`.

## Structured Logging
As `alog` has grown, its use has tended towards usage as part of a multi-replica cluster of servers. In such an environment, it can be very beneficial to provide structure in your log messages so that they can be aggregated between replicas and used for operational visibility. The simplest way to do this is to log lines as `json` rather than the traditional pretty-print formatting. By default `alog` uses the pretty-printer output formatter. To enable JSON output, set `formatter: FormatterKind::Json` in `Config`.

```rust
use alog::{Config, FormatterKind};

alog::configure(Config {
    formatter: FormatterKind::Json,
    ..Default::default()
});
```

While printing logs as `json` allows them to be filtered by `channel`, `level`, and `message` quite easily, some times more structure is needed. In these cases, `alog` also supports logging arbitrary key/value pairs via `alog_map!`, using a [`MapData`] (a `serde_json::Map<String, serde_json::Value>`). For example:

```rust
use alog::{alog_map, MapData, MessageLevel};
use serde_json::json;

let mut extra = MapData::new();
extra.insert("foo".to_string(), json!("bar"));
extra.insert("baz".to_string(), json!(1234));
alog_map!("MAIN", MessageLevel::Info, extra, "some data");
```

## Metadata
In addition to the content of an individual log message, you may want to attach some metadata to all log lines that occur within a given thread of execution. For example, this can be used to attach a request ID to all log lines created in the course of processing a given server request. This can come in very handy when you have a multi-threaded and/or multi-replica environment.

The metadata feature in `alog` is implemented as a thread-local key/value map. Keys and values are added to the map using [`ScopedMetadata`], a guard that adds its keys on construction and removes exactly those keys when it drops, so metadata is always cleaned up even if the scope exits early (including by panic).

Here's a brief example of how you might use metadata:

```rust
use alog::{alog, MapData, MessageLevel, ScopedMetadata};
use serde_json::json;

fn add(a: i32, b: i32) -> i32 {
    alog!("MATH", MessageLevel::Info, "Adding {a} + {b}");
    a + b
}

fn handler(request_id: &str, a: i32, b: i32) -> i32 {
    let mut metadata = MapData::new();
    metadata.insert("request_id".to_string(), json!(request_id));
    let _scope = ScopedMetadata::new(metadata);
    add(a, b)
}
```

In this example, `add` doesn't need (or want) to know that it's part of handling some sort of request, but the developer would like to attach the request ID to the log line printed in its implementation in case there's a bug.

The `let _scope = ScopedMetadata::new(metadata);` binding above can also be written as `alog_scoped_metadata!(metadata);` — see [Log Scopes](#log-scopes) below.

## Logging Macros
The standard logging macros each take a channel, a level, and message arguments:

* `alog!(channel, level, ...)`: Log a single message line using `format!`-style arguments.
    ```rust
    use alog::{alog, MessageLevel};
    alog!("MAIN", MessageLevel::Debug, "This is the {}st test", 1);
    ```

* `alog_map!(channel, level, map, ...)`: Log a single message line with an arbitrary [`MapData`] attached.
    ```rust
    use alog::{alog_map, MapData, MessageLevel};
    alog_map!("MAIN", MessageLevel::Debug, MapData::new(), "map data");
    ```

* `alog_is_enabled!(channel, level)`: Check whether a channel/level combination is enabled without logging, useful for guarding expensive message construction that doesn't fit in a single `format!` call.
    ```rust
    use alog::{alog, alog_is_enabled, MessageLevel};
    if alog_is_enabled!("MAIN", MessageLevel::Debug2) {
        let msg = (0..100).map(|n| n.to_string()).collect::<Vec<_>>().join(",");
        alog!("MAIN", MessageLevel::Debug2, "{msg}");
    }
    ```

In every case, the message/map arguments are only evaluated if the channel/level combination is enabled, so there is no runtime cost to leaving verbose logging statements in performance-critical code paths.

Every macro above that takes a channel has an `_channel`-suffixed sibling (`alog_map_channel!`, `alog_is_enabled_channel!`) that instead uses the channel bound by `use_channel!` in the enclosing module — see [Use Channel](#use-channel) below.

## Log Scopes
One of the most common uses for logging is to note when a certain block of code starts and ends. To facilitate this, `alog` provides scope guards: types whose `Drop` implementation logs when the scope ends. All logging statements which occur between construction and drop are indented, making for a highly readable log, even with very verbose logging.

Each guard type can be used directly via `::new(...)`, bound to a `let _foo = ...;` variable you name yourself, or via a statement-form macro that binds a hidden, hygienic variable for you (so multiple guards can coexist in the same block without you having to invent distinct names for each one). The macro forms are generally preferred; the direct types remain useful when you need to hold onto the guard explicitly (e.g. store it in a struct field) or when working under the [`disable-logging`](#disabling-logging-entirely) feature, where the macros become no-ops but the types stay fully functional.

* [`ScopedLog::new(channel, level, msg)`] / `alog_scoped_block!(channel, level, ...)`: Logs `"BEGIN: {msg}"` immediately and `"END: {msg}"` when the guard drops, indenting everything logged in between.
    ```rust
    use alog::{alog, alog_scoped_block, MessageLevel};
    fn foo(bar: bool) {
        if bar {
            alog_scoped_block!("MAIN", MessageLevel::Debug, "Bar is true!");
            alog!("MAIN", MessageLevel::Debug2, "Getting it done");
        }
    }
    ```

* [`ScopedTimer::new(channel, level, msg)`] / `alog_scoped_timer!(channel, level, ...)`: Times the work done in the current scope and logs the elapsed time with a `duration_ms` field attached when the guard drops.
    ```rust
    use alog::{alog_scoped_timer, MessageLevel};
    fn foo() {
        alog_scoped_timer!("MAIN", MessageLevel::Debug, "heavy_lifting took: ");
        heavy_lifting();
    }
    # fn heavy_lifting() {}
    ```

* `alog_fn!(channel)`: Adds a BEGIN/END indented block at `Trace` level using the enclosing function's name as the message. Unlike the other scope guards, this one is macro-only — there's no separate named type to construct directly, since the function name is captured at the macro's call site.
    ```rust
    use alog::alog_fn;
    fn foo() {
        alog_fn!("MAIN");
        // ...
    }
    ```

* [`ScopedMetadata::new(map)`] / `alog_scoped_metadata!(map)`: Adds a [`MapData`] of key/value pairs that will be attached to every subsequent log line on this thread until the guard drops. See [Metadata](#metadata) above.

* [`ScopedIndent::new()`] / `alog_scoped_indent!()`: Adds a level of indentation to all logging lines within the current scope, without logging anything itself.
    ```rust
    use alog::{alog, alog_scoped_block, alog_scoped_indent, MessageLevel};
    fn foo(bar: bool) {
        if bar {
            alog_scoped_block!("MAIN", MessageLevel::Debug, "Bar is true!");
            alog_scoped_indent!();
            alog!("MAIN", MessageLevel::Debug2, "Getting it done");
        }
    }
    ```

`alog_fn!`, `alog_scoped_block!`, and `alog_scoped_timer!` each have an `_channel`-suffixed sibling (`alog_fn_channel!`, `alog_scoped_block_channel!`, `alog_scoped_timer_channel!`) that drops the explicit channel argument in favor of the one bound by `use_channel!` — see [Use Channel](#use-channel) below. `alog_scoped_indent!` and `alog_scoped_metadata!` have no `_channel` variant since they don't take a channel argument in the first place.

## Use Channel
In the spirit of channels, log entries should be grouped by logical function. To avoid repeating a channel name at every call site, `alog` provides a module-level binding:

* `use_channel!(channel)`: Declares a function in the enclosing module that fixes the channel name for use with the `_channel`-suffixed macros below.
* `alog_channel!(level, ...)`: Like `alog!`, but uses the channel bound by `use_channel!` in the enclosing module instead of taking one explicitly.

```rust
use alog::{alog_channel, use_channel, MessageLevel};

use_channel!("FOO");

fn doit() {
    alog_channel!(MessageLevel::Debug2, "We're doing this!");
}
```

This pattern extends to every user-facing macro that takes a channel argument: `alog_map_channel!`, `alog_is_enabled_channel!`, `alog_fn_channel!`, `alog_scoped_block_channel!`, and `alog_scoped_timer_channel!` are the channel-bound equivalents of `alog_map!`, `alog_is_enabled!`, `alog_fn!`, `alog_scoped_block!`, and `alog_scoped_timer!` respectively, each dropping the leading `channel` argument in favor of the one declared by `use_channel!`.

## Disabling Logging Entirely
For builds where logging should be compiled out entirely (e.g. a size- or performance-sensitive release build), enable the `disable-logging` Cargo feature:

```toml
[dependencies]
alog = { package = "alchemy-logging", version = "...", features = ["disable-logging"] }
```

With this feature enabled, every logging macro (`alog!`, `alog_map!`, `alog_channel!`, `alog_fn!`, `alog_scoped_block!`, `alog_scoped_timer!`, `alog_scoped_indent!`, `alog_scoped_metadata!`, and all of their `_channel` siblings) becomes a no-op, and `alog_is_enabled!`/`alog_is_enabled_channel!` become the compile-time constant `false`. This is a purely additive, opt-in feature — it is off by default, and turning it on never changes behavior beyond silencing these macros.

The `Scoped*` guard types (`ScopedLog`, `ScopedTimer`, `ScopedIndent`, `ScopedMetadata`) are deliberately **not** gated by this feature and remain fully functional when constructed directly, since they're ordinary values your code may depend on structurally (e.g. holding one in a struct field), not just a logging side effect.

## Example
See [`examples/fib.rs`](examples/fib.rs) for a full feature-tour example that exercises channels, levels, filters, both formatters, scoped logs/timers/metadata, and multi-threaded logging. Run it with:

```sh
cargo run --example fib -- 5 8 3
```
