# Alchemy Logging (alog) - Typescript / Javascript
The `alog` framework provides tunable logging with easy-to-use defaults and power-user capabilities. The mantra of `alog` is **"Log Early And Often"**. To accomplish this goal, `alog` makes it easy to enable verbose logging at develop/debug time and trim the verbosity at production run time.

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
There are three primary pieces of configuration when setting up the `alog` environment:

1. **defaultLevel**: This is the level that will be enabled for a given channel when a specific level has not been set in the **filters**.

1. **filters**: This is a mapping from channel name to level that allows levels to be set on a per-channel basis.

1. **formatter**: This is the type of output formatting to use. It defaults to `pretty-print` for ease of readability during development, but can also be configured to log structured `json` records for production logging opviz frameworks.

The `alog.configure()` function allows the default level, filters, and formatter to be set all at once. For example:

```ts
import alog from 'alchemy-logging';

// Set the default level to info with filter overrides for the HTTP and ROUTR
// channels to debug and warning respectively. Configure the formatter to be
// structured json.
alog.configure('info', 'HTTP:debug,ROUTR:warning', 'json');
```

There are several ways to call `configure`, depending on where the values are coming from. The above example shows how it can be called with string input which is best when reading configuration variables from the environment. If programmatically setting the values, the native configuration values can be used as well. Here are some examples:

```ts
// Use the native level value and map syntax
alog.configure(alog.INFO, {HTTP: alog.DEBUG, ROUTR: alog.WARNING});

// Use a configure object
alog.configure({
    defaultLevel: alog.INFO,
    filters: {
        HTTP: alog.DEBUG,
        ROUTR: alog.WARNING,
    },
    formatter: alog.PrettyFormatter,
})
```

### Custom Formatters

For finer-grained control over the formatting of the log records, you can provide a custom formatter function. It must conform to the signature:

```ts
export type FormatterFunc = (logRecord: alog.LogRecord) => string;
```

The most common custom formatter ask is for a version of the `PrettyFormatter` with a different channel-string truncation level. This can be easily achieved with a wrapper around the standard `PrettyFormatter`:

```ts
alog.configure({
    defaultLevel: alog.INFO,
    formatter: (record: alog.LogRecord): string => alog.PrettyFormatter(record, 12),
});
```

### Custom Output Streams

By default `alog` will always log to `process.stdout`. If you need to capture the formatted output log (for example in a log file), you can use `alog.addOutputStream`:

```ts
import alog from 'alchemy-logging';
import { createWriteStream } from 'stream';

alog.configure('debug', '', 'json');

// Add the custom stream to the output file
const logFileStream = createWriteStream('output.json');
alog.addOutputStream(logFileStream);
```

## Logging

Now that the framework has been configured, it's time to actually make log entries!

### Top-Level Log Functions

The simplest way to log information is to use the top-level log functions. For example:

```ts
alog.info('CHANL', 'Hello from alog!');
```

All log functions have several required and optional components:

* `channel` (required): The first argument is always the channel.
* `logCode` (optional): If desired, a `logCode` can be given as the second argument. A log code is a unique string enclosed in `<>` which will identify a specific log record for easy retrieval later. For example:

    ```ts
    alog.info('CHANL', '<LOG12345678I>', 'This is a log that we want to be able to look up in prod');
    ```

* `message` (required): The `message` argument can be either a string or a generator function that takes no arguments and lazily creates the log message. A generator function can be useful when the string is expensive to construct. For example:

    ```ts
    alog.debug4('CHANL', () => {
        let message: string = '[ ';
        for (const entry of bigArray) {
            string += `${entry} `;
        }
        message += ']';
        return message;
    });
    ```

* `metadata` (optional): A map of JSON-serializable `metadata` can be added as the last argument to any log function. For example:

    ```ts
    try {
        thisIsNotGoingToWork();
    } catch (e) {
        alog.warning('CHANL', 'Something is wrong, but life goes on!', {
            error: {
                message: e.message,
                filename: e.fileName,
                line_number: e.lineNumber,
            },
        });
    }
    ```

### Channel Logs

It's often desirable to bind a shared channel name to an object so that it can be reused across a portion of your codebase. To accomplish this, `alog` supports the `useChannel` function which creates a `ChannelLog`. The `ChannelLog` object has a function for each of the log levels which omits the first `channel` argument and instead passes the bound channel name.

```ts
const channel: alog.ChannelLog = alog.useChannel('CHANL');
channel.debug2('Some details');
```

Additionally, a `ChannelLog` supports the `isEnabled` function:

```ts
const channel: alog.ChannelLog = alog.useChannel('CHANL');
if (channel.isEnabled(alog.debug2)) {
    const someNonFunctionalThing = makeLoggingSideEffect();
    channel.debug2(`The side effect is: ${someNonFunctionalThing}`);
}
```

**NOTE**: For complex log creation, the recommended method is to use a `MessageGenerator` to create the message lazily.

### Lazy Logging

The `alog` framework is designed to enable low-level logging without incurring performance hits. To support this, you can use lazy log creation to only perform message creation if the given channel and level are enabled. This can be done by creating a `MessageGenerator`. A `MessageGenerator` is a function that takes no arguments and produces a `string`. For example:

```ts
function expensiveStringCreation() {
    ...
    return someString;
}
alog.info('CHANL', expensiveStringCreation);
```

In this example, the `expensiveStringCreation` function will only be invoked if the `CHANL` channel is enabled at the `info` level.

The most common expensive log creation is done with standard [Template Literals](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals). While expanding a template literal is not terribly expensive for high-level logs, if logs are added to low-level functions, they can add up. The `alog.fmt` function acts as a [Tag](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals#tagged_templates) to keep common template literal syntax while leveraging lazy `MessageGenerator`s. For example:

```ts
largeArray.forEach((element: any) => {
    alog.debug4('CHANL', alog.fmt`The element is ${element}`);
});
```

## Utilities

### Global Metadata

Sometimes there are additional pieces of information that you want attached to all log entries (e.g. an env-based deployment identifier). This is supported via the `alog.addMetadata` and `alog.removeMetadata` functions. For example:

```ts
import alog from 'alchemy-logging';
alog.configure('debug');

if (process.env.DEPLOYMENT_ID) {
    alog.addMetadata('deployment_id', process.env.DEPLOYMENT_ID);
}
```

### Indentation

To support the the mission of making logs that are easy to read at dev-time, `alog` supports the notion of `indentation` which allows the `PrettyFormatter` to display nested logs with easy-to-read indentation. This can be performed manually using `alog.indent` and `alog.deindent`. For example:

```ts
import alog from 'alchemy-logging';
alog.configure('debug3');

function doit(arrayOfStuff) {
    alog.debug('CHANL', 'Doing it!');
    alog.indent();
    arrayOfStuff.forEach((entry) => {
        alog.debug3('CHANL', `Found entry ${entry}`);
    });
    alog.deindent();
    alog.debug('CHANL', 'Done with it');
}

doit([1, 2, 3, 4]);
```

The resulting output looks like

```
2019-11-26T18:34:15.488Z [CHANL:DBUG] Doing it!
2019-11-26T18:34:15.490Z [CHANL:DBG3]   Found entry 1
2019-11-26T18:34:15.490Z [CHANL:DBG3]   Found entry 2
2019-11-26T18:34:15.491Z [CHANL:DBG3]   Found entry 3
2019-11-26T18:34:15.491Z [CHANL:DBG3]   Found entry 4
2019-11-26T18:34:15.491Z [CHANL:DBUG] Done with it
```
