
// Standard
import { Writable } from 'stream';

// Third Party
const deepCopy = require('deepcopy');

// Local
import { AlogConfigError } from './alog_config_error';

// Expose the error types
export { AlogConfigError } from './alog_config_error';

// Constants ///////////////////////////////////////////////////////////////////

export const OFF = 60;
export const FATAL = 59;
export const ERROR = 50;
export const WARNING = 40;
export const INFO = 30;
export const TRACE = 15;
export const DEBUG = 10;
export const DEBUG1 = 9;
export const DEBUG2 = 8;
export const DEBUG3 = 7;
export const DEBUG4 = 6;

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
  if (typeof lvl === 'number') {
    return Number.isInteger(lvl) && lvl > 0;
  } else if (typeof lvl === 'string') {
    return Object.keys(levelFromName).includes(lvl);
  } else {
    return false;
  }
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
  if (configObject.formatter !== undefined) {
    let validFormatter = false;
    if (typeof configObject.formatter === 'function') {
      validFormatter = true;
    } else if (typeof configObject.formatter === 'string'
      && Object.keys(defaultFormatterMap).includes(configObject.formatter)) {
      validFormatter = true;
    }
    if (!validFormatter) {
      return false;
    }
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
  return JSON.stringify(record);
}

const defaultFormatterMap: {[key: string]: FormatterFunc} = {
  pretty: PrettyFormatter,
  json: JsonFormatter,
}

// Core Details ////////////////////////////////////////////////////////////////

// Core singleton class
class AlogCoreSingleton {

  ///////////////////
  // Public Static //
  ///////////////////

  // Public singleton access
  public static getInstance() {
    if (!AlogCoreSingleton.instance) {
      AlogCoreSingleton.instance = new AlogCoreSingleton();
    }
    return AlogCoreSingleton.instance;
  }

  ////////////////////
  // Private Static //
  ////////////////////

  // The singleton instance
  private static instance: AlogCoreSingleton;

  /////////////////////////
  // Private Member Data //
  /////////////////////////

  private defaultLevel: number;
  private filters: FilterMap;
  private formatter: FormatterFunc;
  private numIndent: number;
  private metadata: LogMetadata;
  private streams: Writable[];

  // Private constructor
  private constructor() {
    this.reset();

    // Add log functions for each level
    for (const levelName of Object.keys(levelFromName)) {
      (this as any)[levelName] = (
        channel: string,
        argThree: string|MessageGenerator|LogMetadata,
        argFour?: string|MessageGenerator|LogMetadata,
        argFive?: LogMetadata) => {
        this.log(levelFromName[levelName], channel, argThree, argFour, argFive);
      }
    }
  }

  /////////////////////////////
  // Public Instance Methods //
  /////////////////////////////

  // Set the default level
  public setDefaultLevel(defaultLevel: number): void {
    AlogCoreSingleton.getInstance().defaultLevel = defaultLevel;
  }

  // Set the filters
  public setFilters(filters: FilterMap): void {
    AlogCoreSingleton.getInstance().filters = filters;
  }

  // Set the formatter
  public setFormatter(formatter: FormatterFunc): void {
    AlogCoreSingleton.getInstance().formatter = formatter;
  }

  // Increment the indent level
  public indent(): void {
    AlogCoreSingleton.getInstance().numIndent += 1;
  }

  // Decrement the indent level
  public deindent(): void {
    AlogCoreSingleton.getInstance().numIndent = Math.max(0, AlogCoreSingleton.getInstance().numIndent - 1);
  }

  // Add a metadata key/value pair
  public addMetadata(key: string, value: any): void {
    AlogCoreSingleton.getInstance().metadata[key] = value;
  }

  // Remove a metadata key
  public removeMetadata(key: string): void {
    delete AlogCoreSingleton.getInstance().metadata[key];
  }

  // Add an output stream
  public addOutputStream(stream: Writable): void {
    AlogCoreSingleton.getInstance().streams.push(stream);
  }

  // Reset streams to default
  public resetOutputStreams(): void {
    AlogCoreSingleton.getInstance().streams = [process.stdout];
  }

  // The core channel/level enablement check function
  public isEnabled(channel: string, level: number): boolean {
    const enabledLevel: number = (
      AlogCoreSingleton.getInstance().filters[channel] ||
      AlogCoreSingleton.getInstance().defaultLevel);
    return level >= enabledLevel;
  }

  //////////////////////////////
  // Private Instance Methods //
  //////////////////////////////

  // Reset to default values for all member data
  private reset() {
    this.defaultLevel = OFF;
    this.filters = {};
    this.formatter = defaultFormatterMap.pretty;
    this.numIndent = 0;
    this.metadata = {};

    // Output streams. At minimum send it to stdout
    this.streams = [process.stdout];
  }

  // Level-agnostic log implementation
  //
  // Note that this function implements the following signature options for a
  // given log-level emitter:
  //
  // log(channel: string, logCode: string, message?: MessageGenerator, metadata?: LogMetadata)
  // log(channel: string, logCode: string, message?: string, metadata?: LogMetadata)
  // log(channel: string, messageGenerator: string, metadata?: LogMetadata)
  // log(channel: string, message: string, metadata?: LogMetadata)
  //
  // @param level: The level to emit the log at
  // @param channel: The channel to log on
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
  private log(
    level: number,
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {

    // Only do any work if this level/channel combo is enabled
    if (AlogCoreSingleton.getInstance().isEnabled(channel, level)) {

      // Create the base log record
      const record: LogRecord = {
        channel,
        level,
        level_str: nameFromLevel[level],
        timestamp: new Date().toISOString(),
        message: '',
        num_indent: AlogCoreSingleton.getInstance().numIndent,
      };

      // If there is global metadata configured, add it
      if (Object.keys(AlogCoreSingleton.getInstance().metadata).length) {
        record.metadata = deepCopy(AlogCoreSingleton.getInstance().metadata);
      }

      // Determine if the first variable arg is a log code
      if (isLogCode(argThree)) {
        record.log_code = argThree as string;

        if (typeof argFour === 'function') {
          // Signature 1
          // log(channel: string, logCode: string, message?: MessageGenerator, metadata?: LogRecord)
          record.message = argFour() as string;
        } else if (typeof argFour === 'string') {
          // Signature 2
          // log(channel: string, logCode: string, message?: string, metadata?: LogRecord)
          record.message = argFour as string;
        } // else ignore argFour because it's invalid
        if (argFive !== undefined && Object.keys(argFive).length) {
          record.metadata = Object.assign((record.metadata || {}), deepCopy(argFive));
        }

      } else {

        if (typeof argThree === 'function') {
          // Signature 3
          // log(channel: string, message?: MessageGenerator, metadata?: LogRecord)
          record.message = argThree() as string;
        } else if (typeof argThree === 'string')  {
          // Signature 4
          // log(channel: string, message?: string, metadata?: LogRecord)
          record.message = argThree as string;
        } // else ignore argThree because it's invalid
        if (argFour !== undefined && typeof argFour === 'object' && Object.keys(argFour).length) {
          record.metadata = Object.assign((record.metadata || {}), deepCopy(argFour));
        }
      }

      // Invoke the formatter to get the formatted string
      const logStr: string = AlogCoreSingleton.getInstance().formatter(record);

      // Write to each of the output streams
      AlogCoreSingleton.getInstance().streams.forEach((stream: Writable): void => {
        stream.write(logStr + '\n');
      });
    }
  }
}

// Configure Implementation ////////////////////////////////////////////////////

function levelFromArg(level: string | number): number {
  if (typeof level === 'string') {
    if (isValidLevel(level)) {
      return levelFromName[level];
    } else {
      throw new AlogConfigError(`Invalid level name: [${level}]`);
    }
  } else if (typeof level === 'number') {
    return level;
  } else {
    throw new AlogConfigError(`Invalid argument type: [${typeof level}]`);
  }
}

function filtersFromArg(filters: FilterMap | string): FilterMap {
  if (typeof filters === 'object') {
    if (!isValidFilterConfig(filters)) {
      throw new AlogConfigError(`Invalid filter config: ${JSON.stringify(filters)}`);
    }
    return filters;
  } else if (typeof filters === 'string') {
    // Parse if it's a string
    const parsed: any = {};
    filters.split(',').filter((part) => part.trim() !== '').forEach((part: string) => {
      const keyVal = part.split(':');
      if (keyVal.length !== 2) {
        throw new AlogConfigError(`Invalid filter spec part [${part}]`);
      } else if (!isValidLevel(keyVal[1])) {
        throw new AlogConfigError(`Invalid filter level [${part}]`);
      }
      parsed[keyVal[0]] = levelFromName[keyVal[1]];
    });
    return parsed;
  } else {
    throw new AlogConfigError(`Invalid argument type for filters: [${typeof filters}]`);
  }
}

function formatterFromArg(formatter: FormatterFunc | string) {
  if (typeof formatter === 'function') {
    // If it's a function, just use it (and hope it's a valid one!)
    return formatter;
  } else if (typeof formatter === 'string') {
    // Otherwise, look up in the default formatter map
    if (defaultFormatterMap[formatter] === undefined) {
      throw new AlogConfigError(
        `Invalid formatter type "${formatter}". Options are: [${Object.keys(defaultFormatterMap)}]`);
    } else {
      return defaultFormatterMap[formatter];
    }
  }
}

function parseConfigureArgs(
  argOne: AlogConfig | string | number,
  filters?: FilterMap| string,
  formatter?: string | FormatterFunc): AlogConfig {

  // These are the three core pieces of config we need from the various args
  let parsedDefaultLevel: number = OFF;
  let parsedFilters: { [channelName: string]: number } = {};
  let parsedFormatter: FormatterFunc = defaultFormatterMap.pretty;

  // argOne might be a big map of all the things: (defaultLevel, Filters, formatter)
  if (typeof argOne === "object") {
    if (isValidConfig(argOne)) {

      // If other arguments specified, throw an error
      if (filters !== undefined || formatter !== undefined) {
        throw new AlogConfigError('Cannot configure with an object and explicit filter/formatter args');
      } else {
        parsedDefaultLevel = argOne.defaultLevel;
        parsedFilters = argOne.filters || parsedFilters;

        // If formatter is a string, look it up
        parsedFormatter = (
          (typeof argOne.formatter === 'string' && defaultFormatterMap[argOne.formatter])
          || argOne.formatter)
        || parsedFormatter;

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
    parsedDefaultLevel = levelFromArg(argOne);
  }

  // filters
  if (filters !== undefined) {
    parsedFilters = filtersFromArg(filters);
  }

  // formatter
  if (formatter !== undefined) {
    parsedFormatter = formatterFromArg(formatter);
  }

  // Return the config
  return {
    defaultLevel: parsedDefaultLevel,
    filters: parsedFilters,
    formatter: parsedFormatter,
  };
}

// Core Exports ////////////////////////////////////////////////////////////////

// Map type for channel -> level filters
export interface FilterMap {[channel: string]: number}

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
  filters?: FilterMap;
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
  filters?: FilterMap | string,
  formatter?: string | FormatterFunc) {

  // Configure core singleton to support alog channels and levels
  const instance: AlogCoreSingleton = AlogCoreSingleton.getInstance();

  // Set the current defaults
  const config: AlogConfig = parseConfigureArgs(argOne, filters, formatter);
  instance.setDefaultLevel(config.defaultLevel);
  instance.setFilters(config.filters);
  instance.setFormatter(config.formatter);
}

// Expose parts of the singleton directly
export const indent = AlogCoreSingleton.getInstance().indent;
export const deindent = AlogCoreSingleton.getInstance().deindent;
export const addMetadata = AlogCoreSingleton.getInstance().addMetadata;
export const removeMetadata = AlogCoreSingleton.getInstance().removeMetadata;
export const isEnabled = AlogCoreSingleton.getInstance().isEnabled;
export const addOutputStream = AlogCoreSingleton.getInstance().addOutputStream;
for (const levelName of Object.keys(levelFromName)) {
  exports[levelName] = (AlogCoreSingleton.getInstance() as any)[levelName];
}
