
// Standard
import { Writable } from 'stream';

// Third Party
import MemoryStreams from 'memory-streams';
const deepEqual = require('deep-equal');

/*-- Constants ---------------------------------------------------------------*/

export const sampleLogCode = '<TST00000000I>';

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
