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
import {
  DEBUG,
  DEBUG1,
  DEBUG2,
  DEBUG3,
  DEBUG4,
  ERROR,
  FATAL,
  FormatterFunc,
  INFO,
  MessageGenerator,
  TRACE,
  WARNING,
} from './types';

////////////////////////////////////////////////////////////////////////////////
// Formatter functions take in a record (by reference) and produce a string
// representation. Note that the record MAY be modified in place as the core
// will guarantee that it is a clean copy of any/all data used to construct it.
////////////////////////////////////////////////////////////////////////////////

// Map from level to 4-character string for pretty-print header
const prettyLevelNames: {[key: number]: string} = {
  [FATAL]: "FATL",
  [ERROR]: "ERRR",
  [WARNING]: "WARN",
  [INFO]: "INFO",
  [TRACE]: "TRCE",
  [DEBUG]: "DBUG",
  [DEBUG1]: "DBG1",
  [DEBUG2]: "DBG2",
  [DEBUG3]: "DBG3",
  [DEBUG4]: "DBG4",
}

// The pretty-print representation of an indentation
const prettyIndentation = '  ';

export function PrettyFormatter(record: any, channelLength: number = 5): string {

  //// Make the header ////
  let header: string = '';

  // Timestamp
  header += record.timestamp;

  // Channel
  const chan: string = record.channel.substring(0, channelLength)
    + ' '.repeat(Math.max(0, channelLength - record.channel.length));
  header += ` [${chan}`;

  // Level
  header += `:${prettyLevelNames[record.level] || 'UNKN'}]`;

  // Log Code
  if (record.log_code !== undefined) {
    header += ` ${record.log_code}`;
  }

  // Indent
  header += prettyIndentation.repeat(record.num_indent);

  //// Log each line in the message ////
  let outStr = '';
  for (const line of record.message.split('\n')) {
    outStr += `${header} ${line}\n`;
  }

  //// Add Metadata ////

  if (record.metadata !== undefined) {
    for (const key of Object.keys(record.metadata)) {
      const val: string = JSON.stringify(record.metadata[key]);
      outStr += `${header} * ${key}: ${val}\n`;
    }
  }

  // Strip off the final newline and return
  return outStr.trimRight();
}

export function JsonFormatter(record: any): string {
  // Flatten the metadata into the record and serialize
  if (record.metadata !== undefined) {
    const metadata: any = record.metadata;
    delete record.metadata;
    return JSON.stringify(Object.assign(metadata, record));
  } else {
    return JSON.stringify(record);
  }
}

export const defaultFormatterMap: {[key: string]: FormatterFunc} = {
  pretty: PrettyFormatter,
  json: JsonFormatter,
}

/** @brief The fmt function is a tagged template that lazily creates a message
 * generator
 *
 * This function is syntatic sugar around creating a MessageGenerator to perform
 * lazy logging. It takes advantage of the Tagged Template feature of node to
 * act as a Templat Literal Tag. For example:
 *
 * const val: number = 1;
 * alog.debug('CHAN', alog.fmt`The value is ${val}`);
 *
 * Reference: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals#tagged_templates
 */
export function fmt(template: TemplateStringsArray, ...substitutions: any[]): MessageGenerator {
  return () => String.raw(template, ...substitutions);
}
