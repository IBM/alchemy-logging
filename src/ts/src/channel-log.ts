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
import { LogMetadata, MessageGenerator } from './types';

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

  public fatal(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().fatal(this.channel, argThree, argFour, argFive);
  }

  public error(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().error(this.channel, argThree, argFour, argFive);
  }

  public warning(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().warning(this.channel, argThree, argFour, argFive);
  }

  public info(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().info(this.channel, argThree, argFour, argFive);
  }

  public trace(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().trace(this.channel, argThree, argFour, argFive);
  }

  public debug(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().debug(this.channel, argThree, argFour, argFive);
  }

  public debug1(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().debug1(this.channel, argThree, argFour, argFive);
  }

  public debug2(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().debug2(this.channel, argThree, argFour, argFive);
  }

  public debug3(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().debug3(this.channel, argThree, argFour, argFive);
  }

  public debug4(
    channel: string,
    argThree: string|MessageGenerator|LogMetadata,
    argFour?: string|MessageGenerator|LogMetadata,
    argFive?: LogMetadata,
  ): void {
    AlogCoreSingleton.getInstance().debug4(this.channel, argThree, argFour, argFive);
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
