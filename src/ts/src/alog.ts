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

// Local
import { AlogCoreSingleton } from './core';

// Pass-Through exports
export { configure } from './configure';
export { JsonFormatter, PrettyFormatter } from './formatters';
export {
  AlogConfig,
  AlogConfigError,
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
  LogMetadata,
  LogRecord,
  MessageGenerator,
  OFF,
  TRACE,
  WARNING,
} from './types';

// Exports /////////////////////////////////////////////////////////////////////

// Expose parts of the singleton directly
export const addMetadata = AlogCoreSingleton.getInstance().addMetadata;
export const addOutputStream = AlogCoreSingleton.getInstance().addOutputStream;
export const deindent = AlogCoreSingleton.getInstance().deindent;
export const indent = AlogCoreSingleton.getInstance().indent;
export const isEnabled = AlogCoreSingleton.getInstance().isEnabled;
export const removeMetadata = AlogCoreSingleton.getInstance().removeMetadata;
export const resetOutputStreams = AlogCoreSingleton.getInstance().resetOutputStreams;

// Add all of the level functions
export const fatal = (AlogCoreSingleton.getInstance() as any).fatal;
export const error = (AlogCoreSingleton.getInstance() as any).error;
export const warning = (AlogCoreSingleton.getInstance() as any).warning;
export const info = (AlogCoreSingleton.getInstance() as any).info;
export const trace = (AlogCoreSingleton.getInstance() as any).trace;
export const debug = (AlogCoreSingleton.getInstance() as any).debug;
export const debug1 = (AlogCoreSingleton.getInstance() as any).debug1;
export const debug2 = (AlogCoreSingleton.getInstance() as any).debug2;
export const debug3 = (AlogCoreSingleton.getInstance() as any).debug3;
export const debug4 = (AlogCoreSingleton.getInstance() as any).debug4;
