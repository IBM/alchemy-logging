
// Third Party
const deepCopy = require('deepcopy');

// Local
import { AlogConfigError } from './alog_config_error';

// Expose the error types
export { AlogConfigError } from './alog_config_error';

// Constants ///////////////////////////////////////////////////////////////////

const OFF = 60;
const FATAL = 59;
const ERROR = 50;
const WARNING = 40;
const INFO = 30;
const TRACE = 15;
const DEBUG = 10;
const DEBUG1 = 9;
const DEBUG2 = 8;
const DEBUG3 = 7;
const DEBUG4 = 6;

// Build the map which maps strings -> numeric levels
const levelFromName: {[levelName: string]: number} = {
  off: OFF,
  fatal: FATAL,
  error: ERROR,
  warning: WARNING,
  info: INFO,
  trace: TRACE,
  debug: DEBUG,
  debug1: DEBUG1,
  debug2: DEBUG2,
  debug3: DEBUG3,
  debug4: DEBUG4
};

// Build the map which maps numeric levels -> strings
const nameFromLevel: {[levelValue: number]: string} = {};
Object.keys(levelFromName).forEach((levelName: string) => {
  nameFromLevel[levelFromName[levelName]] = levelName;
});

// Validation //////////////////////////////////////////////////////////////////

function isValidLevel(lvl: any) {
  return Number.isInteger(lvl) && lvl > 0;
}

function isLogCode(arg: any) {
  return typeof arg === 'string' && arg.match(/^<[A-Z]{3}\d{8}[IWDEFT]>$/) !== null;
}

function isValidFilterConfig(filterConfig: any) {
  for (const key of filterConfig) {
    if (!isValidLevel(filterConfig[key])) {
      return false;
    }
  }
  return true;
}

function isValidConfig(configObject: any): boolean {

  // defaultLevel
  if (!isValidLevel(configObject.defaultLevel)) {
    return false;
  }

  // filters
  if (configObject.filters !== undefined) {
    if (typeof configObject.filters !== 'object') {
      return false;
    }
    if (!isValidFilterConfig(configObject.filters)) {
      return false;
    }
  }

  // formatter
  if (configObject.formatter !== undefined && typeof configObject.formatter !== 'function') {
    return false;
  }

  // other

  return true;
}

// Formatters //////////////////////////////////////////////////////////////////

function PrettyFormatter(record: any): string {
  //DEBUG
  return 'THIS IS A STUB !';
}

function JsonFormatter(record: any): string {
  //DEBUG
  return JSON.stringify({stub: 'message'});
}

const defaultFormatterMap: {[key: string]: FormatterFunc} = {
  pretty: PrettyFormatter,
  json: JsonFormatter,
}

// Core Details ////////////////////////////////////////////////////////////////

// Core singleton class
class AlogCoreSingleton {

  // Public //

  // Public singleton access
  public static getInstance() {
    if (!AlogCoreSingleton.instance) {
      AlogCoreSingleton.instance = new AlogCoreSingleton();
    }
    return AlogCoreSingleton.instance;
  }

  // Private //

  // The singleton instance
  private static instance: AlogCoreSingleton;

  // Private member data
  private defaultLevel: number;
  private filters: {[channel: string]: number};
  private formatter: FormatterFunc;
  private indent: number;
  private metadata: LogMetadata;

  // Private constructor
  private constructor() {
    this.defaultLevel = OFF;
    this.filters = {};
    this.formatter = defaultFormatterMap.pretty;
    this.indent = 0;
    this.metadata = {};
  }

  // The core channel/level enablement check function
  private isEnabled(channel: string, level: number): boolean {
    return level > (this.filters[channel] || this.defaultLevel);
  }

  // Emitter implementation
  //
  // Note that this function implements the following signature options for a
  // given log-level emitter:
  //
  // emit(channel: string, logCode: string, message?: MessageGenerator, metadata?: LogMetadata)
  // emit(channel: string, logCode: string, message?: string, metadata?: LogMetadata)
  // emit(channel: string, messageGenerator: string, metadata?: LogMetadata)
  // emit(channel: string, message: string, metadata?: LogMetadata)
  //
  // @param channel: The channel to log on
  // @param level: The level to emit the log at
  // @param argThree: This can be one of several things:
  //  * log code - Add that to the record as the log_code
  //  * string - Just log it
  //  * MessageGenerator - Lazy generator for the message
  //  * LogMetadata - Key value map of metadata objects
  // @param argFour: This can be one of several things:
  //  * string - If argThree is a log code, this is the message
  //  * MessageGenerator - If argThree is a log code, this is a generator for
  //      the message
  //  * LogMetadata - Metadata for the call
  // @param argFive: If argThree is a log code and argFour is either a string or
  //  a MessageGenerator, this can be extra metadata for the call
  private emit(
    channel: string,
    level: number,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {

    // Only do any work if this level/channel combo is enabled
    if (this.isEnabled(channel, level)) {

      // Create the base log record
      const record: LogRecord = {
        channel,
        level,
        level_str: nameFromLevel[level],
        timestamp: new Date().toISOString(),
        message: '',
        num_indent: this.indent,
      };

      // If there is global metadata configured, add it
      if (Object.keys(this.metadata).length) {
        record.metadata = deepCopy(this.metadata);
      }

      // Determine if the first variable arg is a log code
      if (isLogCode(argThree)) {
        record.log_code = argThree as string;

        if (typeof argFour === 'function') {
          // Signature 1
          // emit(channel: string, logCode: string, message?: MessageGenerator, metadata?: LogRecord)
          record.message = argFour() as string;
        } else if (typeof argFour === 'string') {
          // Signature 2
          // emit(channel: string, logCode: string, message?: string, metadata?: LogRecord)
          record.message = argFour as string;
        } // else ignore argFour because it's invalid
        if (argFive !== undefined && Object.keys(argFive).length) {
          record.metadata = Object.assign((record.metadata || {}), deepCopy(argFive));
        }

      } else {

        if (typeof argThree === 'function') {
          // Signature 3
          // emit(channel: string, message?: MessageGenerator, metadata?: LogRecord)
          record.message = argThree() as string;
        } else if (typeof argThree === 'string')  {
          // Signature 4
          // emit(channel: string, message?: string, metadata?: LogRecord)
          record.message = argThree as string;
        } // else ignore argThree because it's invalid
        if (argFour !== undefined && typeof argFour === 'object' && Object.keys(argFour).length) {
          record.metadata = Object.assign((record.metadata || {}), deepCopy(argFour));
        }
      }

      // Invoke the formatter to get the formatted string
      const logStr: string = this.formatter(record);

      // Write it to stdout
      process.stdout.write(logStr);
    }
  }
}

function parseConfigureArgs(
  argOne: AlogConfig | string,
  filters?: {[key: string]: number} | string,
  formatter?: string | FormatterFunc): AlogConfig {

  // These are the three core pieces of config we need from the various args
  let parsedDefaultLevel: number = OFF;
  let parsedFilters: { [channelName: string]: number };
  let parsedFormatter: FormatterFunc = defaultFormatterMap.pretty;

  // argOne might be a big map of all the things: (defaultLevel, Filters, formatter)
  if (typeof argOne === "string") {
    if (levelFromName.hasOwnProperty(argOne)) {
      parsedDefaultLevel = levelFromName[argOne];
    } else {
      throw new AlogConfigError(`Invalid level string [${argOne}]`);
    }
  } else if (typeof argOne === "number") {
    if (isValidLevel(argOne)) {
      parsedDefaultLevel = argOne;
    } else {
      throw new AlogConfigError(`Invalid level value [${argOne}]`);
    }
  } else if (typeof argOne === "object") {
    if (isValidConfig(argOne)) {

      // If other arguments specified, throw an error
      if (filters !== undefined || formatter !== undefined) {
        throw new AlogConfigError('Cannot configure with an object and explicit filter/formatter args');
      } else {
        parsedDefaultLevel = argOne.defaultLevel;
        parsedFilters = argOne.filters || parsedFilters;
        parsedFormatter = argOne.formatter || parsedFormatter;

        // Return the config directly with defaults
        return {
          defaultLevel: parsedDefaultLevel,
          filters: parsedFilters,
          formatter: parsedFormatter,
        };
      }
    } else {
      throw new AlogConfigError(`Invalid config object: ${JSON.stringify(argOne)}`);
    }
  } else {
    throw new AlogConfigError(`Invalid argument type: [${typeof argOne}]`);
  }

  // filters
  if (filters !== undefined) {
    // Validate if it's an object
    if (typeof filters === 'object') {
      if (!isValidFilterConfig(filters)) {
        throw new AlogConfigError(`Invalid filter config: ${JSON.stringify(filters)}`);
      }
      parsedFilters = filters;
    } else if (typeof filters === 'string') {
      // Parse if it's a string
      const parsed: any = {};
      filters.split(',').forEach((part: string) => {
        const keyVal = part.split(':');
        if (keyVal.length !== 2) {
          throw new AlogConfigError(`Invalid filter spec part [${part}]`);
        } else if (!isValidLevel(keyVal[1])) {
          throw new AlogConfigError(`Invalid filter level [${part}]`);
        }
        parsed[keyVal[0]] = levelFromName[keyVal[1]];
      });
      parsedFilters = parsed;
    } else {
      throw new AlogConfigError(`Invalid argument type for filters: [${typeof filters}]`);
    }
  }

  // formatter
  if (formatter !== undefined) {
    if (typeof formatter === 'function') {
      // If it's a function, just use it (and hope it's a valid one!)
      parsedFormatter = formatter;
    } else if (typeof formatter === 'string') {
      // Otherwise, look up in the default formatter map
      if (defaultFormatterMap[formatter] === undefined) {
        throw new AlogConfigError(
          `Invalid formatter type "${formatter}". Options are: [${Object.keys(defaultFormatterMap)}]`);
      } else {
        parsedFormatter = defaultFormatterMap[formatter];
      }
    }
  }

  // Return the config
  return {
    defaultLevel: parsedDefaultLevel,
    filters: parsedFilters,
    formatter: parsedFormatter,
  };
}

// Core Exports ////////////////////////////////////////////////////////////////

// Artibrary metadata dict
export interface LogMetadata {[key: string]: any}

// An alog record can be any arbitrary key/val map
export interface LogRecord {
  // Required fields
  channel: string;
  level: number;
  level_str: string;
  timestamp: string;
  message: string;
  num_indent: number;

  // Specific optional fields
  log_code?: string;
  metadata?: LogMetadata;
}

// The type for a custom formatter function
export type FormatterFunc = (logRecord: LogRecord) => string;

// The type for a lazy record generator
export type MessageGenerator = () => string;

export interface AlogConfig {
  defaultLevel: number;
  filters?: {[channelName: string]: number};
  formatter?: FormatterFunc;
}

/** @brief Configure the core singleton and set the config for which are
 * enabled/disabled
 *
 * This is the primary setup function for alog. It must be called before any of
 * the other alog functions are called, but it may be re-invoked at any time to
 * update the configuration.
 *
 * @param argOne: This argument can either be a full AlogConfig object, or the
 *    string name for the defaultLevel
 * @param filters: This argument can either be a map from channel name (string)
 *    to configured level (number) or a string in the format
 *    'CHAN1:level,CHAN2:level' that will be parsed into the corresponding map
 * @param formatter: This argument can either be a formatter function, or the
 *    string name of a predefined formatter ('pretty' or 'json')
 */
export function configure(
  argOne: AlogConfig | string,
  filters?: {[key: string]: number} | string,
  formatter?: string | FormatterFunc) {

  // Configure core singleton to support alog channels and levels

  // Set the current defaults

}

// Feature Priorities //////////////////////////////////////////////////////////
/*

MUST HAVE:
* Full set of levels
* Multi-channel level setting
* Pretty and JSON formatting
* Lazy message construction
* (core detail) custom output stream, mostly for testing

CORE FEATURES:
* channel.with_metadata()
* Scoped indentation
* Function trace BEGIN/END
* Some way to produce complex logs lazily

NICE FEATURES:
* Scoped timing
* Decorators for scope behavior
* ALOG_USE_CHANNEL for classes

*/


// Examples ////////////////////////////////////////////////////////////////////


// configure();
// sampleLogger = bunyan.createLogger({name: 'test_app', level: DEBUG1});

// // Put these things into a real testing dir if any of this actually works.
// Bunyan should be able to resolve custom levels.



// // const alog = require('alog');
// alog.configure('debug', 'MAIN:debug4');
// alog.configure('debug', { MAIN: 'debug4' });
// alog.configure(alog.DEBUG, {MAIN: alog.DEBUG4});
// alog.configure({
//   defaultLevel: alog.DEBUG,
//   filters: {
//     MAIN: alog.DEBUG4
//   },
//   formatter: 'json' // 'pretty', CustomFormatter(),
//   // threaId: true
// });

// const channel = alog.useChannel('MAIN');

// // 1. Log code
// // 2. Message generator (string, function, string + format args)
// // 3. Metadata map
// //
// // log_code? message [format_args]...

// channel.debug2('<TST12345678D>', 'This is the %dst test', 1);
// channel.withMetadata({
//   metakey: 'val',
// }).info('<TST12345678I>', 'This is a test');
// channel.debug4('<TST12345678D>', () => {
//   let m = '';
//   myContainer.forEach((element) => m += '--' + element);
//   return m;
// });
