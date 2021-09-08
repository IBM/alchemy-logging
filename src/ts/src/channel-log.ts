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

// Local
import { AlogCoreSingleton } from './core';
import { LogCode, Loggable, LogMetadata } from './types';

/**
 * The ChannelLog object binds a channel name and wraps all of the core
 * functions which are scoped to an individual channel.
 */
export class ChannelLog {

  public channel: string;
  private coreInstance: AlogCoreSingleton;

  constructor(channel: string) {
    this.channel = channel;
    this.coreInstance = AlogCoreSingleton.getInstance();
  }

  /*-- Log Functions --*/

  // Log to fatal
  public fatal(message: Loggable, metadata?: LogMetadata): void;
  public fatal(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public fatal(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().fatal(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to error
  public error(message: Loggable, metadata?: LogMetadata): void;
  public error(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public error(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().error(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to warning
  public warning(message: Loggable, metadata?: LogMetadata): void;
  public warning(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public warning(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().warning(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to info
  public info(message: Loggable, metadata?: LogMetadata): void;
  public info(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public info(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().info(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to trace
  public trace(message: Loggable, metadata?: LogMetadata): void;
  public trace(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public trace(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().trace(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to debug
  public debug(message: Loggable, metadata?: LogMetadata): void;
  public debug(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public debug(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().debug(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to debug1
  public debug1(message: Loggable, metadata?: LogMetadata): void;
  public debug1(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public debug1(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().debug1(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to debug2
  public debug2(message: Loggable, metadata?: LogMetadata): void;
  public debug2(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public debug2(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().debug2(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to debug3
  public debug3(message: Loggable, metadata?: LogMetadata): void;
  public debug3(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public debug3(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().debug3(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  // Log to debug4
  public debug4(message: Loggable, metadata?: LogMetadata): void;
  public debug4(logCode: LogCode, message?: Loggable, metadata?: LogMetadata): void;
  public debug4(
    codeOrMsg: LogCode|Loggable,
    msgOrMeta?: Loggable|LogMetadata,
    meta?: LogMetadata,
  ) {
    AlogCoreSingleton.getInstance().debug4(
      this.channel, codeOrMsg as string, msgOrMeta as Loggable, meta,
    );
  }

  /*-- Helper Functions --*/

  public isEnabled(level: number): boolean {
    return this.coreInstance.isEnabled(this.channel, level);
  }
};

/** @brief Factory wrapper for creating a new channel */
export function useChannel(channel: string): ChannelLog {
  return new ChannelLog(channel);
}
