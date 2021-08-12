# Implementation Specification

- [Backgrond](#background)
- [Design Summary](#design-summary)
    - [Channels And Levels](#channels-and-levels)
    - [Configuration](#configuration)
    - [Output Formatting](#output-formatting)
- [Details](#details)
    - [1 Interface](#1-interface)
        - [1.1 Configuration](#11-configuration)
        - [1.2 Record Objects](#12-record-objects)
            - [1.2.1 Required fields](#121-required-fields)
            - [1.2.2 Optional fields](#122-optional-fields)
            - [1.2.3 Additional keys and values](#123-additional-keys-and-values)
        - [1.3 Log Record Creation](#13-log-record-creation)
            - [1.3.1 Individual text records](#131-individual-text-records)
            - [1.3.2 Map records](#132-map-records)
            - [1.3.3 Channel log objects and functions](#133-channel-log-objects-and-functions)
            - [1.3.4 Indentation](#134-indentation)
        - [1.4 Pretty Formatting](#14-pretty-formatting)
            - [1.4.1 Pretty header](#141-pretty-header)
            - [1.4.2 Line breaks](#142-line-breaks)
            - [1.4.3 Map data](#143-map-data)
        - [1.5 Json Formatting](#15-json-formatting)
        - [1.6 Extra Functions](#16-extra-functions)
            - [1.6.1 Thread logs](#161-thread-logs)
            - [1.6.2 Metadata](#162-metadata)
            - [1.6.3 Scoped logs](#163-scoped-logs)
            - [1.6.4 Function trace scopes](#164-function-trace-scopes)
            - [1.6.5 Timed scopes](#165-timed-scopes)
    - [2 Implementation](#2-implementation)
        - [2.1 Core](#21-core)
        - [2.2 Lazy Message Construction](#22-lazy-message-construction)
        - [2.3 Thread Safety](#23-thread-safety)
        - [2.4 Dynamic Configuration](#24-dynamic-configuration)
        - [2.5 Minimal Dependencies](#25-minimal-dependencies)
        - [2.6 Language Native Semantics](#26-language-native-semantics)



# Background

In modern cloud-based microservice architectures, one of the most common pieces of functionality needed is operational visibility at runtime. One of the primary tools used is application logging which allows developers to write useful logging messages to an output stream that are specific to the application itself. Application logs are typically used both as a tool for developers when writing applications, and as a tool for system administrators when analyzing the behavior of applications in a production system.

Numerous logging frameworks exist in the open-source community that can create log records at runtime, however current frameworks have several key limitations:

1. Each framework is limited to a single (or handful) of programming languages, causing a lack of standard formatting across microservices implemented in different programming languages.

2. Most frameworks are limited to filtering records by a sequential set of levels, resulting in very coarse-grained filtering at runtime. This typically results in an all-or-nothing environment where there are very few log messages in an "info" level, but far too many to understand on a "debug" level.

3. Log records are generated either as structured key/value objects or plain-text strings. Structured records lead to ease of aggregation and search, but are very hard for developers to visually scan and understand when diagnosing problems in the system. Plain-text logs have the opposite problem of being easy to read, but hard to aggregate and search across a distributed system.

This invention aims to solve these shortcomings of the existing logging frameworks. The stated goals of this invention are:

1. Provide consistent semantics for creating, formatting, and configuring log records across applications built in multiple programming languages with a specification for adding new languages.

2. Provide a filtering mechanism for log records at runtime that allows developers to log as verbosely as they find useful during development, while limiting clutter during production runtime.

3. Provide standard formatting for log records that can easily be converted between a structured key/value object for a production runtime environment, and a human-readable pretty-print format for manual debugging and parsing by developers.

# Design Summary

The proposed logging framework has the following key tenants:

1. The specification for the framework is agnostic to the programming language it is implemented in, but the language-specific implementations may adapt the interface to feel native in the specific language.

2. All implementations of the framework must implement a set of core concepts for consistency.

The core components of the framework are:

## Channels And Levels:

The primary components of the system are channels and levels which allow for each log statement to be enabled or disabled when appropriate. Each logging statement is made to a specific channel and level. Channels are independent of one another and allow for logical grouping of log messages by functionality. A channel can be any string. Levels provide sequential granularity, allowing detailed debugging statements to be placed in the code without clogging up the logs at runtime. The sequence of levels and their general usage is as follows:

* `off`: Disable the given channel completely
* `fatal`: A fatal error has occurred. Any behavior after this statement should be regarded as undefined.
* `error`: An unrecoverable error has occurred. Any behavior after this statement should be regarded as undefined unless the error is explicitly handled.
* `warning`: A recoverable error condition has come up that the service maintainer should be aware of.
* `info`: High-level information that is valuable at runtime under moderate load.
* `trace`: Used to log begin/end of functions for debugging code paths.
* `debug`: High-level debugging statements such as function parameters.
* `debug1`: High-level debugging statements.
* `debug2`: Mid-level debugging statements such as computed values.
* `debug3`: Low-level debugging statements such as computed values inside loops.
* `debug4`: Ultra-low-level debugging statements such as data dumps and/or statements inside multiple nested loops.

## Configuration:

There are three primary pieces of configuration when setting up the environment:

* `default_level`: This is the level that will be enabled for a given channel when a specific level has not been set in the filters.

* `filters`: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

* `format`: Whether to format the output as json or pretty-printed lines.

## Output Formatting:

There are two output formats supported: Pretty and json.

* `pretty`: Easily visible logging for quick visual parsing. This includes aligned header blocks so that messages can easily be read in sequence and optional indentation to represent scoped blocks.
* `json`: Formatted logging for systemic log data creation

# Details

## 1 Interface

This section outlines the language-agnostic interface that all implementations of the framework must implement.

### 1.1 Configuration

The framework must have a single configuration function which, at a minimum, takes arguments for default_level, filters, and format as described below:

* `default_level`: Either an enumeration value representing one of the available levels, or a string representation of the corresponding level (e.g. "info")
* `filters`: A mapping from channel names (strings) to levels. This can be represented via a language-native map data structure, or a string of the format "CHAN1:level,CHAN2:level"
* `format`: A string or enumeration value corresponding to either the "pretty" formatter or the "json" formatter. Languages may also implement the option to specify a user-defined formatter function for further customization. The default value for this argument must be "pretty."

In addition to these required configuration arguments, implementations may also include the following:

* `thread_id`: Enable adding a unique identifier for the thread where the record was created to the record itself for all records ([see 1.6.1](#161-thread-logs))
* `metadata`: Enable adding cross-cutting metadata key/value pairs to records via a set of extra functions ([see 1.6.2](#162-metadata))

### 1.2 Record Objects

At its core, each logging statement creates a single record object. A record object is a key/value map consisting of a set of required and optional string keys with values that follow strict semantics.

#### 1.2.1 Required fields

The following fields are required in every log record and must be created by the implementation of the logging function:

* `channel (string)`: The string representation of the named channel on which the record was created.
* `level (enum)`: The numeric enumeration value of the severity level on which the record was created.
* `level_str (string)`: The string label for the severity level on which the record was created.
* `timestamp (string)`: A string-formatted representation of the time when the record was created. All implementations should format the timestamp following [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) standard (`YYYY-MM-DDTHH:mm:ss.sssZ`).
* `message (string)`: The free-text message for the specific record.
* `num_indent (unsigned integer)`: The number of indentation levels present when the record was created ([see 1.3.4](#134-indentation)).

#### 1.2.2 Optional fields

* `thread_id (string)`: A unique identifier for the thread on which the record was created.
* `log_code (string)`: A unique string used to identify the specific log message.

#### 1.2.3 Additional keys and values

In addition to the above, a record may contain arbitrary additional key/value pairs as long as they follow the json type restrictions (see [ECMA-404](https://www.ecma-international.org/publications-and-standards/standards/ecma-404/)).

### 1.3 Log Record Creation

The framework supports a number of ways to create log records with various forms of content. This section outlines the primary ways that a record may be created that must be present in all implementations.

#### 1.3.1 Individual text records

All implementations must provide globally-available free functions (or macros) to create individual log records at any point in a body of source code where the framework has been imported or included. While the naming and formatting of the functions can be implemented to match the language, all implementations must take the following arguments:

* `channel (string)`: The channel on which to create this record.
* `level (enum)`: The severity level at which to create this record.
* `message (string)`: The free-text message to log. Depending on the language-specific implementation of lazy message construction ([see 2.2](#22-lazy-message-construction)), this may optionally take a list of formatting arguments to be evaluated into a log message if and only if the channel/level combination is enabled.

#### 1.3.2 Map records

For some circumstances, application logic dictates that a log message should contain a formatted set of keys and values in a single record. To support this, all implementations must optionally allow an arbitrary map of json-compatible key/value pairs to be given to the individual logging free functions.

#### 1.3.3 Channel log objects and functions

In order to support consistency in the set of channels used by a given application, and to eliminate sources of human error where programmers may mis-type a channel name, every implementation must support a mechanism to declare a channel name once that will be shared by all logging function invocations within a given logical context. There are three primary mechanisms to support this that may be applicable to a given language's implementation:

* **USE_CHANNEL macro**: For languages that support macros, a `USE_CHANNEL` macro may declare and implement a member function for a given class that returns a fixed channel name for all logging invocations within other member functions of the same class.

* **USE_CHANNEL_FREE macro**: For languages that support macros, a `USE_CHANNEL_FREE` macro may declare a free function in a given program scope that returns a fixed channel name for all other free functions in the same scope. Where possible, this must be restricted to a single compilation/runtime unit.

* **Log channel object**: For languages without macro support, functionality can be implemented to create an instance of an object that has a bound channel name and supports individual logging functions without a channel argument. Such an object may be held as a member variable of a class or as a free object in a given compilation/runtime unit (depending on the implementation language).

In addition to the above methods for declaring a channel, every implementation must support a set of logging functions analogous to those in [1.3.1](#131-individual-text-records) and [1.3.2](#132-map-records) which do not require the channel argument, but act instead on the configured channel. For languages where a macro implementation is used, these functions may simply invoke the named function created by the macro. For languages using the log channel object implementation, these functions may be member functions of the channel object.

#### 1.3.4 Indentation

Every log record must be created with an indentation value that may be used by the formatter to display nested log messages based on the logical scope of the record. The default indentation should always be 0. Various mechanisms for managing indentation can be provided by the implementation:

* **Free functions**: At a minimum, each implementation must provide free functions (or macros) to increment and decrement the indentation level. These functions may be invoked in code to cause all subsequent records to have an increased or decreased indentation level.

* **Scope objects**: For languages that support it, object classes may be created that increment the indentation level at construction and decrement it at destruction. This can be particularly useful in languages which support stack-scoped objects (such as `c++`) which will automatically destroy an object when it goes out of scope, triggering a decrement in the indentation level. For more details, see ([1.6.3](#163-scoped-logs), [1.6.4](#164-function-trace-scopes), and [1.6.5](#165-timed-scopes)).

### 1.4 Pretty Formatting

All implementations of the framework must implement a formatting mechanism that takes a single record and formats it as one or more lines of string output. This formatting method is primarily intended for use by developers when actively developing the code. It allows for easy visual parsing of highly-detailed log messages produced either by experimental run invocations or unit tests.

#### 1.4.1 Pretty header

Each line in the pretty-formatted output must contain a header at the beginning. The header block is designed to be a fixed-width block that is compact enough to view alongside a raw message in a viewing window (such as a unix terminal) sized to a reasonable width. Headers should be formatted as follows:

```
"timestamp [channel:level(:thread_id)] (log_code )(indentation)"
```

The components of the header are as follows:

* `timestamp`: The [ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) formatted timestamp string

* `channel`: A fixed-length representation of the channel name. Since channels can be arbitrary strings, the formatter implementation must either pad or truncate the names to a fixed length (typically 5 characters).

* `level`: The string representation of the severity level. All level strings must be exactly 5 characters long (or be padded where appropriate such as with `"INFO"`).

* `thread_id`: When enabled, all headers must contain the unique string identifier for the thread where the record was created. When possible, the thread id must be padded or truncated to a fixed with to maintain the fixed width of the header.

* `log_code`: If log codes are supported by the implementation and present for the given log record, they should be added immediately after the square bracket and before the indentation. All log codes must be fixed-length.

* `indentation`: The indentation section is a fixed number of space characters (typically 2) multiplied by the `num_indent` for the individual record.

#### 1.4.2 Line breaks

For messages containing multiple lines, the message should be split on line breaks and written out as multiple pretty-printed lines, each with its own copy of the header for the specific record. The timestamp should not be recomputed for for these split lines.

#### 1.4.3 Map data

When a record contains arbitrary key/value pairs, either specified at creation ([see 1.3.2](#132-map-records)) or via metadata ([see 1.6.2](#162-metadata)), the pretty-print formatter should add a single line for each entry after the body of the free-text message has been logged. Each line must contain the header block and be formatted as follows:

```
" * key: value"
```

Values should be serialized as single-line json, so that primitive types are represented cleanly and nested lists or key/value objects are still serialized on a single line.

### 1.5 Json Formatting

In addition to the pretty-print formatter, all implementations must have an output formatter which serializes each record to a single string formatted following the json standard ([ECMA-404](https://www.ecma-international.org/publications-and-standards/standards/ecma-404/)).

### 1.6 Extra Functions

In addition to the functionality outlined above, there are a number of optional features that may be added to a given language-specific implementation, but should remain consistent across languages when available.

#### 1.6.1 Thread logs

In addition to the configuration option to enable `thread_id` strings to be added for all records ([see 1.2.2](#122-optional-fields)), implementations may provide free functions which always add a `thread_id` to a specific record when created. This can be particularly useful in sections of a codebase dealing with sensitive multithreading concerns.

#### 1.6.2 Metadata

In addition to the ability to specify arbitrary json-compatible key/value pairs in an individual log record ([see 1.3.2](#132-map-records)), implementations may provide functionality for adding and removing metadata key/value pairs which apply to all subsequent records until removed either by a manual removal or by the close of a scope object. This can be particularly useful in the context of a microservice which may want to attach a unique identifier to all records produced by a given request to the server.

#### 1.6.3 Scoped logs

One of the most common uses for debug-level logging is tracking when a given routine begins and ends in order to debug problems that occur inside the block. To support this, implementations may have class objects which produce logical scoping in the log output. These object must be constructed with a free-text message and create records at construction time and destruction time. The format of the scope messages is:

* At construction: `"BEGIN: (message)"`
* At destruction: `"END: (message)"`

In addition to the free-text message, each scope object must take a channel and level at construction time. In keeping with the notion of lazy message construction ([see 2.2](#22-lazy-message-construction)), the begin and end records should only be created if the given channel/level combination is enabled for the running configuration. Where possible, implementations may omit storage of the free-text message if the given channel/level is disabled to avoid unnecessary string manipulation and copying.

#### 1.6.4 Function trace scopes

The most frequently used scope in many programming languages is an individual function. For languages that support it, a convenience class may be added that wraps the basic scoped log implementation and constructs the free-text message automatically by introspecting the signature of the function in which the object is created. This can be particularly useful in codebases where following the call flow is difficult and use of a standard debugging tool to interrupt runtime is impractical.

#### 1.6.5 Timed scopes

Another common use for scoping logic in complex codebases is runtime profiling. To support this, an implementation of this framework may support a scoped timer class which starts a timer at construction and creates a log record with the total time since construction upon destruction.

The record created at destruction time should contain a free-text message with the time-delta formatted for easy consumption, including rescaling the value form the native counter value to a human readable value (e.g. nanoseconds -> milliseconds for durations > 1ms). In addition to the free-text message, the record should contain a `"duration_ms"` key whose value is the floating-point number of milliseconds of the duration. The combination of free-text and numeric outputs keeps the message easily visible for developers while making it easily consumable in structured format for more complex profiling tasks.

## 2 Implementation

In general, implementations of this logging framework should attempt to leverage language-native logging frameworks under the hood where possible. For example, an implementation in `python` should leverage the native `logging` package for it's core singleton logic, rather than reimplementing it. For languages where no runtime-native logging framework exists, the following principles should be used.

### 2.1 Core

The core of the logging framework revolves around a [singleton object](https://en.wikipedia.org/wiki/Singleton_pattern) which holds the default enabled level along with the mapping from channel to level for individual channel overrides. A singleton is used so that configuration can be performed globally for a running application. The corresponding configuration will apply to all log records created after the invocation. The core singleton is responsible for the logic to determine whether a given channel is enabled at a given level. In some languages, the core singleton may also contain a member function that consolidates the implementation of all log record creation.

### 2.2 Lazy Message Construction

One of the core tenants of most modern application logging frameworks is lazy evaluation. Manipulation of strings into log messages can be an expensive operation, particularly in performance-sensitive areas of code, so many frameworks implement lazy construction such that any dynamic message construction is done only if the given record is enabled by the core configuration. All implementations of this framework here must follow this paradigm. As an example, in `c++`, the framework may implemented using macros which place the string construction code into the conditional block that is only accessed if the given channel is enabled at the given level. For languages without macro support, such as `python`, formatting and interpolation operations may be deferred to only execute inside a conditional statement using a standard format function and passing the format string and arguments to the record creation function.

### 2.3 Thread Safety

In addition to the configuration information, the core singleton should hold the output formatter(s) and stream(s). Access to these resources should be managed by read and write locks appropriately for the threading infrastructure of the given language so that records created from different execution threads can be safely formatted and output in sequence. When possible, the operation of writing to the stream should be done outside of the main thread to avoid blocking application code on completion of the write operation.

### 2.4 Dynamic Configuration

Implementations may allow the configuration function to be invoked multiple times to support changing the configuration at runtime. This can be particularly useful in a microservice deployment where maintainers may need to view debug-level logging statements for a running system in order to diagnose an active incident without rebooting the server(s). If an implementation allows for dynamic configuration changes, the corresponding resources in the core singleton must be guarded by a read lock to avoid race conditions when multiple configuration invocations happen in parallel.

### 2.5 Minimal Dependencies

As a general principle, logging frameworks should be lightweight and non-invasive. In order to facilitate this, all implementations of this framework should strive to avoid requiring additional non-standard dependencies. For example, in `python`, the implementation should not require that any additional packages at runtime outside of those that are packaged with the base `python` runtime.

### 2.6 Language Native Semantics

The goal of this framework is to provide a consistent experience for both developers and maintainers of a microservice architecture with multiple components written in multiple programming languages. For developers, it is also important that using the logging framework in the language that any given component is written in feels native to that language. This includes cosmetic details like naming and formatting conventions such as using snake case method names in `python` or using dot-delimited import naming in java. Additional details such as object creation/deletion should be tailored to the standard practices of the language of the implementation.
