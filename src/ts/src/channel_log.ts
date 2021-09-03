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

/**
 * The ChannelLog object binds a channel name and wraps all of the core
 * functions which are scoped to an individual channel.
 */
export class ChannelLog {

  public channel: string;
  private coreInstance: any;

  constructor(channel: string) {
    this.channel = channel;
    this.coreInstance = AlogCoreSingleton.getInstance() as any;
  }

  /*-- Log Functions --*/

  public fatal(...args: any[]): void {
    this.coreInstance.fatal(this.channel, ...args);
  }
  public error(...args: any[]): void {
    this.coreInstance.error(this.channel, ...args);
  }
  public warning(...args: any[]): void {
    this.coreInstance.warning(this.channel, ...args);
  }
  public info(...args: any[]): void {
    this.coreInstance.info(this.channel, ...args);
  }
  public trace(...args: any[]): void {
    this.coreInstance.trace(this.channel, ...args);
  }
  public debug(...args: any[]): void {
    this.coreInstance.debug(this.channel, ...args);
  }
  public debug1(...args: any[]): void {
    this.coreInstance.debug1(this.channel, ...args);
  }
  public debug2(...args: any[]): void {
    this.coreInstance.debug2(this.channel, ...args);
  }
  public debug3(...args: any[]): void {
    this.coreInstance.debug3(this.channel, ...args);
  }
  public debug4(...args: any[]): void {
    this.coreInstance.debug4(this.channel, ...args);
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
