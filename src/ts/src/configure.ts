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
import { defaultFormatterMap } from './formatters';
import {
  AlogConfig,
  AlogConfigError,
  FilterMap,
  FormatterFunc,
  OFF,
} from './types';

// Validation //////////////////////////////////////////////////////////////////

function isValidLevel(lvl: any) {
  if (typeof lvl === 'number') {
    return Number.isInteger(lvl) && lvl > 0;
  } else if (typeof lvl === 'string') {
    return Object.keys(AlogCoreSingleton.levelFromName).includes(lvl);
  } else {
    return false;
  }
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

  return true;
}

// Configure Implementation ////////////////////////////////////////////////////

function levelFromArg(level: string | number): number {
  if (typeof level === 'string') {
    if (isValidLevel(level)) {
      return AlogCoreSingleton.levelFromName[level];
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

  if (filters === null || filters === undefined) {
    return {};
  } else if (typeof filters === 'object') {
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
      parsed[keyVal[0]] = AlogCoreSingleton.levelFromName[keyVal[1]];
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

// Main configure //////////////////////////////////////////////////////////////

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
  argOne: AlogConfig | string | number,
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
