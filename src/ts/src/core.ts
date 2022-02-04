/*------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2021 IBM
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *----------------------------------------------------------------------------*/

// Standard
import { Writable } from 'stream';

// Third Party
const deepCopy = require('deepcopy');

// Local
import { defaultFormatterMap } from './formatters';
import {
  DEBUG,
  DEBUG1,
  DEBUG2,
  DEBUG3,
  DEBUG4,
  ERROR,
  FATAL,
  FilterMap,
  FormatterFunc,
  INFO,
  LogCode,
  Loggable,
  LogMetadata,
  LogRecord,
  MessageGenerator,
  OFF,
  TRACE,
  WARNING,
} from './types';

// Core Details ////////////////////////////////////////////////////////////////

// Core singleton class
export class AlogCoreSingleton {

  ///////////////////
  // Public Static //
  ///////////////////

  // Build the map which maps strings -> numeric levels
  public static levelFromName: {[levelName: string]: number} = {
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
    debug4: DEBUG4,
  };

  // Build the map which maps numeric levels -> strings
  public static nameFromLevel: {[levelValue: number]: string}
    = Object.keys(AlogCoreSingleton.levelFromName)
    .reduce((obj: any, levelName: string) => {
      obj[AlogCoreSingleton.levelFromName[levelName]] = levelName;
      return obj;
    }, {});

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

  private static isLogCode(arg: any) {
    return typeof arg === 'string' && arg.match(/^<.*>$/) !== null;
  }

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

  // Log to fatal
  public fatal(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public fatal(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public fatal(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.fatal, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to error
  public error(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public error(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public error(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.error, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to warning
  public warning(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public warning(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public warning(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.warning, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to info
  public info(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public info(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public info(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.info, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to trace
  public trace(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public trace(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public trace(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.trace, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to debug
  public debug(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public debug(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public debug(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.debug, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to debug1
  public debug1(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public debug1(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public debug1(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.debug1, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to debug2
  public debug2(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public debug2(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public debug2(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.debug2, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to debug3
  public debug3(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public debug3(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public debug3(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.debug3, channel, codeOrMsg, msgOrMeta, meta,
    );
  }

  // Log to debug4
  public debug4(
    channel: string, message: Loggable, metadata?: LogMetadata,
  ): void;
  public debug4(
    channel: string, logCode: LogCode, message?: Loggable, metadata?: LogMetadata,
  ): void;
  public debug4(
    channel: string,
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().log(
      AlogCoreSingleton.levelFromName.debug4, channel, codeOrMsg, msgOrMeta, meta,
    );
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
  // 1. log(channel: string, logCode: string, message?: MessageGenerator, metadata?: LogMetadata)
  // 2. log(channel: string, logCode: string, message?: string, metadata?: LogMetadata)
  // 3. log(channel: string, logCode: string, message?: Error, metadata?: LogMetadata)
  // 4. log(channel: string, logCode: string, metadata?: LogMetadata)
  // 5. log(channel: string, message: MessageGenerator, metadata?: LogMetadata)
  // 6. log(channel: string, message: string, metadata?: LogMetadata)
  // 7. log(channel: string, message: Error, metadata?: LogMetadata)
  // 8. log(channel: string, metadata: LogMetadata)
  //
  // @param level: The level to emit the log at
  // @param channel: The channel to log on
  // @param msgOrMeta: This can be one of several things:
  //  * log code - Add that to the record as the log_code
  //  * string - Just log it
  //  * MessageGenerator - Lazy generator for the message
  //  * LogMetadata - Key value map of metadata objects
  // @param meta: This can be one of several things:
  //  * string - If msgOrMeta is a log code, this is the message
  //  * MessageGenerator - If msgOrMeta is a log code, this is a generator for
  //      the message
  //  * LogMetadata - Metadata for the call
  // @param argFive: If msgOrMeta is a log code and meta is either a string or
  //  a MessageGenerator, this can be extra metadata for the call
  private log(
    level: number,
    channel: string,
    msgOrMeta: Loggable|LogMetadata,
    meta?: Loggable|LogMetadata,
    argFive?: LogMetadata,
  ): void {

    // Only do any work if this level/channel combo is enabled
    if (AlogCoreSingleton.getInstance().isEnabled(channel, level)) {

      // Create the base log record
      //
      // NOTE: The order of the fields here will correspond to their serialized
      //  json order
      const record: LogRecord = {
        timestamp: new Date().toISOString(),
        channel,
        level,
        level_str: AlogCoreSingleton.nameFromLevel[level],
        num_indent: AlogCoreSingleton.getInstance().numIndent,
        message: '',
      };

      // If there is global metadata configured, add it as a clean copy
      if (Object.keys(AlogCoreSingleton.getInstance().metadata).length) {
        record.metadata = deepCopy(AlogCoreSingleton.getInstance().metadata);
      }

      // Determine if the first variable arg is a log code
      if (AlogCoreSingleton.isLogCode(msgOrMeta)) {
        record.log_code = msgOrMeta as string;

        if (typeof meta === 'function') {
          // Signature 1
          // log(channel: string, logCode: string, message?: MessageGenerator, metadata?: LogRecord)
          record.message = meta() as string;
        } else if (typeof meta === 'string') {
          // Signature 2
          // log(channel: string, logCode: string, message?: string, metadata?: LogRecord)
          record.message = meta as string;
        } else if (meta instanceof Error) {
          // Signature 3
          // log(channel: string, logCode: string, message?: Error, metadata?: LogMetadata)
          record.message = (meta as Error).toString();
          record.stack = (meta as Error).stack;
        } else if (typeof meta === 'object') {
          record.message = '';
          record.metadata = Object.assign((record.metadata || {}), deepCopy(meta));
        } // else ignore msgOrMeta because it's invalid
        if (argFive !== undefined && Object.keys(argFive).length) {
          record.metadata = Object.assign((record.metadata || {}), deepCopy(argFive));
        }

      } else {

        if (typeof msgOrMeta === 'function') {
          // Signature 5
          // log(channel: string, message?: MessageGenerator, metadata?: LogRecord)
          record.message = msgOrMeta() as string;
        } else if (typeof msgOrMeta === 'string')  {
          // Signature 6
          // log(channel: string, message?: string, metadata?: LogRecord)
          record.message = msgOrMeta as string;
        } else if (msgOrMeta instanceof Error) {
          // Signature 7
          // log(channel: string, message?: Error, metadata?: LogMetadata)
          record.message = (msgOrMeta as Error).toString();
          record.stack = (msgOrMeta as Error).stack;
        } else if (typeof msgOrMeta === 'object') {
          // Signature 8
          // log(channel: string, metadata: LogMetadata)
          record.message = '';
          record.metadata = Object.assign((record.metadata || {}), deepCopy(msgOrMeta));
        } // else ignore msgOrMeta because it's invalid
        if (meta !== undefined && meta !== null && typeof meta === 'object' && Object.keys(meta).length) {
          record.metadata = Object.assign((record.metadata || {}), deepCopy(meta));
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
