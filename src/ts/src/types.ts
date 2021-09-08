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

// The type used for a log code
export type LogCode = string;

// The type that defines something which can be logged
export type Loggable = string|MessageGenerator;

export interface AlogConfig {
  defaultLevel: number;
  filters?: FilterMap;
  formatter?: FormatterFunc;
}

// Error raised when configuration error occurs
export class AlogConfigError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'AlogConfigError';
  }
};

// The core levels and their numeric equivalents
export const OFF: number     = 60;
export const FATAL: number   = 59;
export const ERROR: number   = 50;
export const WARNING: number = 40;
export const INFO: number    = 30;
export const TRACE: number   = 15;
export const DEBUG: number   = 10;
export const DEBUG1: number  = 9;
export const DEBUG2: number  = 8;
export const DEBUG3: number  = 7;
export const DEBUG4: number  = 6;
