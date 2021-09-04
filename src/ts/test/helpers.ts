
// Standard
import { Writable } from 'stream';

// Third Party
import MemoryStreams from 'memory-streams';
const deepCopy = require('deepcopy');
const deepEqual = require('deep-equal');

// Alog guts
import alog from '../src';
import { AlogCoreSingleton } from '../src/core';
const formatters = require('rewire')('../src/formatters');
const prettyLevelNames = formatters.__get__('prettyLevelNames');
const prettyIndentation = formatters.__get__('prettyIndentation');

const levelFromName = AlogCoreSingleton.levelFromName;
const nameFromLevel = AlogCoreSingleton.nameFromLevel;

/*-- Constants ---------------------------------------------------------------*/

// Sample log code so that the check_unique_log_codes script doesn't pick up the
// same value in multiple places
export const sampleLogCode = '<TST00000000I>';

// Sentinel string for validation functions to just check for the presence of a
// given key
export const IS_PRESENT = '__is_present__';

/*-- Helpers -----------------------------------------------------------------*/

// How do you log errors when testing a logger??
export function testFailureLog(msg: string): void {
  process.stderr.write(`** ${msg} \n`);
}

// Helper that validates an indivitual log record against expected values.
// If the value in the expected record is IS_PRESENT, then it just checks
// for the presence of the field, otherwise, it checks for equality.
export function validateLogRecord(record: any, expected: any): boolean {

  let res = true;

  // Validate all expected keys
  for (const expKey of Object.keys(expected)) {
    const expValue = expected[expKey];
    if (expValue === IS_PRESENT) {
      if (!record.hasOwnProperty(expKey)) {
        testFailureLog(`Missing expected key ${expKey}`);
        res = false;
      }
    } else if (expKey === 'metadata' && ! deepEqual(expValue, record.metadata)) {
      testFailureLog(
        `Record mismatch [metadata]. Exp: ${JSON.stringify(expValue)}, Got: ${JSON.stringify(record.metadata)}`);
      res = false;
    } else if (expKey !== 'metadata' && record[expKey] !== expValue) {
      testFailureLog(`Record mismatch [${expKey}]. Exp: ${expValue}, Got: ${record[expKey]}`);
      res = false;
    }
  }

  // Make sure there are no unexpected keys
  for (const gotKey of Object.keys(record)) {
    if (!Object.keys(expected).includes(gotKey)) {
      testFailureLog(`Got unexpected key: ${gotKey}`);
      res = false;
    }
  }

  return res;
}

// Wrapper to run validateLogRecord against multiple lines
export function validateLogRecords(records: any[], expecteds: any[]): boolean {

  let res = true;

  // Make sure the lengths are the same
  if (records.length !== expecteds.length) {
    testFailureLog(`Length mismatch. Exp: ${expecteds.length}, Got: ${records.length}`);
    res = false;
  }

  // Iterate the lines and compare in parallel
  const iterSize = Math.min(records.length, expecteds.length);
  for (let i = 0; i < iterSize; i++) {
    if (!validateLogRecord(records[i], expecteds[i])) {
      testFailureLog(`Line mismatch on ${i}`);
      res = false;
    }
  }
  return res;
}

// Get a list of records from a writable stream
export function getLogRecords(logStream: Writable): string[] {
  return logStream.toString().split('\n').filter(
    (line) => line !== '').map(
    (line) => JSON.parse(line));
}

// Helper to create a stub record for validation where all default fields are
// just set to IS_PRESENT
export function stubValidationRecord(): any {
  return deepCopy({
    channel: IS_PRESENT,
    level: IS_PRESENT,
    level_str: IS_PRESENT,
    timestamp: IS_PRESENT,
    message: IS_PRESENT,
    num_indent: IS_PRESENT,
  });
}

// Helper to directly serialize a record to json with no reformatting
export function DirectJsonFormatter(record: any): string {
  return JSON.stringify(record);
}

//// Pretty Print Regexes ////
//
// These regexes are used to parse out the various parts of a pretty-printed
// line. For clarity, they're broken up here into subparts.
////
// e.g. "2019-11-25T22:48:12.993Z"
const ppTimestampRegex = /([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3}Z)/;

// e.g. "[CHANN:INFO] <TST00001000I>     "
const ppHeaderRegex = /\[([^:]*):([^\]:]*)\]( ?<[^\s]*>)? ([\s]*)/;

// e.g. "2019-11-25T22:48:12.993Z [CHANN:INFO] <TST00001000I>   "
const ppFullHeaderRegex = new RegExp(`^${ppTimestampRegex.source} ${ppHeaderRegex.source}`);

// e.g. "This is a test"
const ppMessageRegex = /([^\s].*)\n?/;

// e.g. "* key: val"
const ppMetadataRegex = /\* (.+): (.+)\n?/;

// e.g.  "2019-11-25T22:48:12.993Z [CHANN:INFO] <TST00001000I>   This is a test"
const ppMessageLineRegex = new RegExp(`${ppFullHeaderRegex.source}${ppMessageRegex.source}$`);

// e.g.  "2019-11-25T22:48:12.993Z [CHANN:INFO] <TST00001000I>   * key: val"
const ppMetadataLineRegex = new RegExp(`${ppFullHeaderRegex.source}${ppMetadataRegex.source}$`);

// Inverted mapping from level shortname to numeric value
const levelFromPrettyName: {[prettyName: string]: number} = {};
Object.keys(prettyLevelNames).forEach((levelName: string) => {
  levelFromPrettyName[prettyLevelNames[levelName]] = levelFromName[levelName];
});

// Parse a pretty-print line into a record. The special field `is_metadata` will
// be populated to indicate whether it is a metadata line or not
export function parsePPLine(line: string): any {

  // Try to match first as metadata, then as a normal line
  let isMetadata: boolean = true;
  let match = line.match(ppMetadataLineRegex);
  if (!match) {
    isMetadata = false;
    match = line.match(ppMessageLineRegex);
  }
  if (!match) {
    const msg = `Un-parsable pretty-print line: [${line}]`;
    testFailureLog(msg);
    throw new Error(msg)
  }

  // Add the parts of the header that are always there
  const record: any = {
    is_metadata: isMetadata,
    timestamp: match[1],
    channel: match[2],
    level: levelFromPrettyName[match[3]],
    level_str: nameFromLevel[levelFromPrettyName[match[3]]],
  };

  // log code
  if (match[4]) {
    record.log_code = match[4].trimLeft();
  }

  // indent
  record.num_indent = match[5].length / prettyIndentation.length;

  // message or metadata
  if (isMetadata) {
    const key = match[6];
    const val = JSON.parse(match[7]);
    record[key] = val;
  } else {
    record.message = match[6];
  }

  return record;
}

// Helper to make a record for testing
export function makeTestRecord(overrides?: any): any {
  return Object.assign({
    timestamp: '2019-11-25T22:48:12.993Z',
    channel: 'TEST',
    level: alog.INFO,
    level_str: nameFromLevel[alog.INFO],
    num_indent: 0,
    message: 'asdf',
  }, overrides);
}
