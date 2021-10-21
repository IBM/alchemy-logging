
// Standard
import { Writable } from 'stream';

// Third Party
import { expect } from 'chai';
import MemoryStreams from 'memory-streams';
const deepEqual = require('deep-equal');

// Test helpers
import {
  DirectJsonFormatter,
  getLogRecords,
  IS_PRESENT,
  makeTestRecord,
  parsePPLine,
  sampleLogCode,
  validateLogRecords,
} from './helpers';

// Things under test (will be monkey patched)
import alog from '../src';
import { JsonFormatter, PrettyFormatter } from '../src/formatters';
const rewire = require('rewire');
const configure  = rewire('../src/configure');
const core  = rewire('../src/core');
const AlogCoreSingleton = core.AlogCoreSingleton;
const isLogCode = AlogCoreSingleton.isLogCode;
const levelFromName = AlogCoreSingleton.levelFromName;
const nameFromLevel = AlogCoreSingleton.nameFromLevel;

/*-- Tests -------------------------------------------------------------------*/

describe('Alog TypeScript Internals Test Suite', () => {
  // core singleton suite
  describe('AlogCoreSingleton', () => {

    describe('mutators', () => {
      const alogCore = AlogCoreSingleton.getInstance();
      beforeEach(() => {
        alogCore.reset();
      });

      it('should be able to set the default level', () => {
        expect(alogCore.defaultLevel).to.equal(alog.OFF);
        alogCore.setDefaultLevel(alog.DEBUG);
        expect(alogCore.defaultLevel).to.equal(alog.DEBUG);
      });

      it('should be able to set filters', () => {
        expect(alogCore.defaultLevel).to.equal(alog.OFF);
        alogCore.setFilters({TEST: alog.INFO});
        expect(alogCore.filters).to.deep.equal({TEST: alog.INFO});
      });

      it('should be able to set the formatter', () => {
        expect(alogCore.formatter.name).to.equal(PrettyFormatter.name);
        alogCore.setFormatter(JsonFormatter);
        expect(alogCore.formatter.name).to.equal(JsonFormatter.name);
      });

      it('should be able to indent', () => {
        expect(alogCore.numIndent).to.equal(0);
        alogCore.indent();
        expect(alogCore.numIndent).to.equal(1);
      });

      it('should be able to deindent', () => {
        expect(alogCore.numIndent).to.equal(0);
        alogCore.indent();
        expect(alogCore.numIndent).to.equal(1);
        alogCore.deindent();
        expect(alogCore.numIndent).to.equal(0);
      });

      it('should be not able to deindent past 0', () => {
        expect(alogCore.numIndent).to.equal(0);
        alogCore.deindent();
        expect(alogCore.numIndent).to.equal(0);
      });

      it('should be able to add metadata', () => {
        expect(alogCore.metadata).to.deep.equal({});
        alogCore.addMetadata('key', {nested: 1});
        expect(alogCore.metadata).to.deep.equal({key: {nested: 1}});
      });

      it('should be able to remove metadata', () => {
        expect(alogCore.metadata).to.deep.equal({});
        alogCore.addMetadata('key', {nested: 1});
        expect(alogCore.metadata).to.deep.equal({key: {nested: 1}});
        alogCore.removeMetadata('key');
        expect(alogCore.metadata).to.deep.equal({});
      });

      it('should ignore request to remove unknown metadata', () => {
        expect(alogCore.metadata).to.deep.equal({});
        alogCore.addMetadata('key', {nested: 1});
        expect(alogCore.metadata).to.deep.equal({key: {nested: 1}});
        alogCore.removeMetadata('foobar');
        expect(alogCore.metadata).to.deep.equal({key: {nested: 1}});
      });

      it('should be able to add a custom stream', () => {
        expect(alogCore.streams.length).to.equal(1);
        alogCore.addOutputStream(new MemoryStreams.WritableStream());
        expect(alogCore.streams.length).to.equal(2);
      });

      it('should be able to reset output streams', () => {
        expect(alogCore.streams.length).to.equal(1);
        alogCore.addOutputStream(new MemoryStreams.WritableStream());
        expect(alogCore.streams.length).to.equal(2);
        alogCore.resetOutputStreams();
        expect(alogCore.streams.length).to.equal(1);
      });
    }); // mutators

    describe('isEnabled', () => {
      const alogCore = AlogCoreSingleton.getInstance();
      beforeEach(() => {
        alogCore.reset();
        alogCore.setDefaultLevel(alog.DEBUG);
        alogCore.setFilters({LOWER: alog.INFO, HIGHER: alog.DEBUG2});
      });

      it('should return enabled false using the default level', () => {
        expect(alogCore.isEnabled('TEST', alog.DEBUG4)).to.be.false;
      });

      it('should return enabled true using the default level', () => {
        expect(alogCore.isEnabled('TEST', alog.DEBUG)).to.be.true;
      });

      it('should return enabled false using a filter lower than the default', () => {
        expect(alogCore.isEnabled('LOWER', alog.DEBUG)).to.be.false;
      });

      it('should return enabled true using a filter lower than the default', () => {
        expect(alogCore.isEnabled('LOWER', alog.WARNING)).to.be.true;
      });

      it('should return enabled false using a filter higher than the default', () => {
        expect(alogCore.isEnabled('HIGHER', alog.DEBUG3)).to.be.false;
      });

      it('should return enabled true using a filter higher than the default', () => {
        expect(alogCore.isEnabled('HIGHER', alog.DEBUG1)).to.be.true;
      });
    }); // isEnabled

    describe('log', () => {

      const alogCore = AlogCoreSingleton.getInstance();
      let logStream: Writable;
      beforeEach(() => {
        alogCore.reset();
        alogCore.setDefaultLevel(alog.DEBUG);
        alogCore.setFilters({LOWER: alog.INFO, HIGHER: alog.DEBUG2});
        alogCore.setFormatter(DirectJsonFormatter);
        logStream = new MemoryStreams.WritableStream();
        alogCore.addOutputStream(logStream);
      });

      it('should be able to log with signature 1', () => {
        alogCore.log(alog.DEBUG, 'TEST', sampleLogCode, () => 'This is a generated message', {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', sampleLogCode, () => 'This is a second generated message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is a generated message',
            metadata: {foo: 'bar'},
            log_code: sampleLogCode,
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is a second generated message',
            log_code: sampleLogCode,
          },
        ])).to.be.true;
      });

      it('should be able to log with signature 2', () => {
        alogCore.log(alog.DEBUG, 'TEST', sampleLogCode, 'This is NOT a generated message', {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', sampleLogCode, 'This is NOT a second generated message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is NOT a generated message',
            metadata: {foo: 'bar'},
            log_code: sampleLogCode,
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is NOT a second generated message',
            log_code: sampleLogCode,
          },
        ])).to.be.true;
      });

      it('should be able to log with signature 3', () => {
        alogCore.log(alog.DEBUG, 'TEST', () => 'This is a generated message', {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', () => 'This is a second generated message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is a generated message',
            metadata: {foo: 'bar'},
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is a second generated message',
          },
        ])).to.be.true;
      });

      it('should be able to log with signature 4', () => {
        alogCore.log(alog.DEBUG, 'TEST', 'This is NOT a generated message', {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', 'This is NOT a second generated message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is NOT a generated message',
            metadata: {foo: 'bar'},
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is NOT a second generated message',
          },
        ])).to.be.true;
      });

      it('should correctly log merged global and local metadata', () => {
        alogCore.addMetadata('baz', 1);
        alogCore.log(alog.DEBUG, 'TEST', 'This is NOT a generated message', {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', 'This is NOT a second generated message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is NOT a generated message',
            metadata: {foo: 'bar', baz: 1},
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'This is NOT a second generated message',
            metadata: {baz: 1},
          },
        ])).to.be.true;
      });
    }); // log

    describe('level log functions', () => {

      const alogCore = AlogCoreSingleton.getInstance();
      let logStream: Writable;
      beforeEach(() => {
        alogCore.reset();
        alogCore.setDefaultLevel(alog.DEBUG);
        alogCore.setFilters({LOWER: alog.WARNING, HIGHER: alog.DEBUG2});
        alogCore.setFormatter(DirectJsonFormatter);
        logStream = new MemoryStreams.WritableStream();
        alogCore.addOutputStream(logStream);
      });

      it('should have all the expected level functions', () => {
        for (const levelName of Object.keys(levelFromName)) {
          if (levelName === 'off') {
            expect(alog).to.not.have.property(levelName);
          } else {
            expect(alog).to.have.property(levelName);
          }
        }
      });

      it('should log with a level-function when enabled by the default level', () => {
        alogCore.debug('TEST', 'Some fun message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'Some fun message',
          },
        ])).to.be.true;
      });

      it('should not log with a level-function when disabled by the default level', () => {
        alogCore.debug3('TEST', 'Some fun message');
        expect(validateLogRecords(getLogRecords(logStream), [])).to.be.true;
      });

      it('should log with a level-function when enabled by the filters', () => {
        alogCore.debug2('HIGHER', 'Some fun message');
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'HIGHER', level: alog.DEBUG2, level_str: nameFromLevel[alog.DEBUG2],
            timestamp: IS_PRESENT, num_indent: 0,
            message: 'Some fun message',
          },
        ])).to.be.true;
      });

      it('should not log with a level-function when disabled by the filters', () => {
        alogCore.info('LOWER', 'Some fun message');
        expect(validateLogRecords(getLogRecords(logStream), [])).to.be.true;
      });
    });

  }); // AlogCoreSingleton

  // pretty print suite
  describe('PrettyFormatter', () => {

    it('should correctly format a basic line without metadata or log_code', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({
        message: 'This is a test',
      }));
      const parsed: any = parsePPLine(formatted);
      expect(parsed.is_metadata).to.be.false;
      expect(parsed).to.have.property('timestamp');
      expect(parsed).to.have.property('channel');
      expect(parsed).to.have.property('level');
      expect(parsed).to.have.property('level_str');
      expect(parsed).to.have.property('num_indent');
      expect(parsed.message).to.equal('This is a test');
      expect(parsed).to.not.have.property('log_code');
    });

    it('should format a line with a long channel by truncating it', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({channel: 'LONG-NAME'}));
      const parsed: any = parsePPLine(formatted);
      expect(parsed.channel).to.equal('LONG-');
    });

    it('should format a line with a shoft channel by padding it', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({channel: 'SHRT'}));
      const parsed: any = parsePPLine(formatted);
      expect(parsed.channel).to.equal('SHRT ');
    });

    it('should format with the correct number of indentations', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({num_indent: 2}));
      const parsed: any = parsePPLine(formatted);
      expect(parsed.num_indent).to.equal(2);
    });

    it('should format a line with a log code correctly', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({log_code: sampleLogCode}));
      const parsed: any = parsePPLine(formatted);
      expect(parsed.log_code).to.equal(sampleLogCode);
    });

    it('should format a line with a log code and indentation correctly', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({
        log_code: sampleLogCode, num_indent: 3,
      }));
      const parsed: any = parsePPLine(formatted);
      expect(parsed.log_code).to.equal(sampleLogCode);
      expect(parsed.num_indent).to.equal(3);
    });

    it('should correctly split a multi-line record and add the header to each line', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({
        message: `This is a test
with a second line
and a third!`}));
      const formattedLines = formatted.split('\n');
      expect(formattedLines.length).to.equal(3);
      expect(parsePPLine(formattedLines[0]).message).to.equal('This is a test');
      expect(parsePPLine(formattedLines[1]).message).to.equal('with a second line');
      expect(parsePPLine(formattedLines[2]).message).to.equal('and a third!');
    });

    it('should correctly format metadata as a list after the main line', () => {
      const formatted: string = PrettyFormatter(makeTestRecord({
        message: 'This is a test', metadata: {
          key: 1,
          foo: 'bar',
          baz: {
            biz: ['buz'],
          }
        }}));
      const formattedLines = formatted.split('\n');
      expect(formattedLines.length).to.equal(4);
      expect(parsePPLine(formattedLines[0]).message).to.equal('This is a test');
      expect(parsePPLine(formattedLines[1]).is_metadata).to.be.true;
      expect(parsePPLine(formattedLines[1])).to.not.have.property('message');
      expect(parsePPLine(formattedLines[1]).key).to.equal(1);
      expect(parsePPLine(formattedLines[2]).is_metadata).to.be.true;
      expect(parsePPLine(formattedLines[2])).to.not.have.property('message');
      expect(parsePPLine(formattedLines[2]).foo).to.equal('bar');
      expect(parsePPLine(formattedLines[3]).is_metadata).to.be.true;
      expect(parsePPLine(formattedLines[3])).to.not.have.property('message');
      expect(parsePPLine(formattedLines[3]).baz).to.deep.equal({biz: ['buz']});
    });

    it('should format an Error with message and stack', () => {
      const err = new Error('barf');
      const formatted: string[] = PrettyFormatter(makeTestRecord({
        message: err.toString(),
        stack: err.stack,
      })).split('\n');
      // NOTE: The stack string includes the message, so it is skipped as to not
      //  duplicate the error message printed when the message is printed
      const stackLines = err.stack.split('\n');
      expect(formatted.length).to.equal(stackLines.length);
      expect(parsePPLine(formatted[0]).message).to.equal(err.toString());
      let i = 1;
      for (const line of stackLines.slice(1)) {
        expect(parsePPLine(formatted[i]).message).to.equal(line.trim());
        i += 1;
      }
    });
  });

  // json suite
  describe('JsonFormatter', () => {

    it('should serialize a record without metadata correctly', () => {
      const formatted: string = JsonFormatter(makeTestRecord({
        message: 'This is a test',
      }));
      const parsed: any = JSON.parse(formatted);
      expect(parsed).to.have.property('timestamp');
      expect(parsed).to.have.property('channel');
      expect(parsed).to.have.property('level');
      expect(parsed).to.have.property('level_str');
      expect(parsed).to.have.property('num_indent');
      expect(parsed.message).to.equal('This is a test');
      expect(parsed).to.not.have.property('log_code');
    });

    it('should flatten metadata into the main record when serializing', () => {
      const formatted: string = JsonFormatter(makeTestRecord({
        metadata: {
          key: 1,
          foo: 'bar',
          baz: {
            biz: ['buz'],
          },
        },
      }));
      const parsed: any = JSON.parse(formatted);
      expect(parsed.key).to.equal(1);
      expect(parsed.foo).to.equal('bar');
      expect(parsed.baz).to.deep.equal({biz: ['buz']});
    });
  });

  // input validation suite
  describe('Input validation suite', () => {
    // Checks for verifying that log codes are valid or invalid
    describe('Log Code Validation', () => {
      it('A happy log code is valid', () => {
        expect(isLogCode('<ORC12345678D>')).to.be.true;
      });

      it('A log code that is missing its starting angle bracket should fail', () => {
        expect(isLogCode('ORC12345678D>')).to.be.false;
      });

      it('A log code that is missing its closing angle bracket should fail', () => {
        expect(isLogCode('<ORC12345678D')).to.be.false;
      });
    });
  });

  // configuration
  describe('configure', () => {
    describe('isValidLevel', () => {
      const isValidLevel = configure.__get__('isValidLevel');
      it('should accept any positive number', () => {
        expect(isValidLevel(999)).to.be.true;
      });
      it('should reject a non-positive number', () => {
        expect(isValidLevel(0)).to.be.false;
        expect(isValidLevel(-1)).to.be.false;
      });
      it('should reject a floating point number', () => {
        expect(isValidLevel(3.14159)).to.be.false;
      });
      it('should accept a level string', () => {
        expect(isValidLevel('debug')).to.be.true;
      });
      it('should reject a non-level string', () => {
        expect(isValidLevel('foobar')).to.be.false;
      });
      it('should reject a non-number non-string', () => {
        expect(isValidLevel({})).to.be.false;
      });
    });

    describe('isValidFilterConfig', () => {
      const isValidFilterConfig = configure.__get__('isValidFilterConfig');
      it ('should accept an empty filter object', () => {
        expect(isValidFilterConfig({})).to.be.true;
      });
      it ('should accept a filter config with valid values', () => {
        expect(isValidFilterConfig({ FOO: 'debug', BAR: 123 })).to.be.true;
      });
      it ('should reject a filter config with an invalid value', () => {
        expect(isValidFilterConfig({ FOO: 'debug', BAR: -1 })).to.be.false;
      });
    });

    describe('isValidConfig', () => {
      const isValidConfig = configure.__get__('isValidConfig');
      it('should accept a valid config with all fields set', () => {
        expect(isValidConfig({
          defaultLevel: alog.INFO,
          filters: { FOO: alog.DEBUG, BAR: alog.WARNING },
          formatter: (record: alog.LogRecord): string => JSON.stringify(record),
        })).to.be.true;
      });
      it('should accept a valid config with a formatter name', () => {
        expect(isValidConfig({
          defaultLevel: alog.INFO,
          formatter: 'pretty',
        })).to.be.true;
      });
      it('should accept a valid config with no optional fields set', () => {
        expect(isValidConfig({ defaultLevel: alog.INFO })).to.be.true;
      });
      it('should reject a config with an invalid default level', () => {
        expect(isValidConfig({ defaultLevel: -10 })).to.be.false;
      });
      it('should reject a config with an invalid filter map', () => {
        expect(isValidConfig({
          defaultLevel: alog.INFO,
          filters: { FOO: -10, BAR: alog.WARNING },
        })).to.be.false;
      });
      it('should reject a config with a filter that is not an object', () => {
        expect(isValidConfig({
          defaultLevel: alog.INFO,
          filters: 1 as any,
        })).to.be.false;
      });
      it('should reject a config with an invalid formatter', () => {
        expect(isValidConfig({
          defaultLevel: alog.INFO,
          formatter: 'badstring',
        })).to.be.false;
      });
    });

    describe('levelFromArg', () => {
      const levelFromArg = configure.__get__('levelFromArg');
      it('should parse a valid string level', () => {
        expect(levelFromArg('info')).to.equal(alog.INFO);
      });
      it('should throw on an invalid string level', () => {
        expect(() => levelFromArg('foobar'))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('should parse a valid number level', () => {
        expect(levelFromArg(alog.INFO)).to.equal(alog.INFO);
      });
      it('should throw on an invalid number level', () => {
        expect(() => levelFromArg(-1))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('should throw on an invalid level type', () => {
        expect(() => levelFromArg(['info']))
          .to.throw().with.property('name', 'AlogConfigError');
      });
    });

    describe('filtersFromArg', () => {
      const filtersFromArg = configure.__get__('filtersFromArg');
      it('should handle null and undefined as empty objects', () => {
        expect(filtersFromArg(null)).to.deep.equal({});
        expect(filtersFromArg(undefined)).to.deep.equal({});
      });
      it('should handle a valid empty object', () => {
        expect(filtersFromArg({})).to.deep.equal({});
      });
      it('should handle a valid object with entries', () => {
        const cfg = {FOO: alog.INFO};
        expect(filtersFromArg(cfg)).to.deep.equal(cfg);
      });
      it('should throw on an invalid object', () => {
        expect(() => filtersFromArg({FOO: -1}))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('should parse an empty string as an empty config', () => {
        expect(filtersFromArg('')).to.deep.equal({});
      });
      it('should parse a valid string with multiple entries', () => {
        expect(filtersFromArg('FOO:info,BAR:debug'))
          .to.deep.equal({FOO: alog.INFO, BAR: alog.DEBUG});
      });
      it('should throw on a string with an invalid level value', () => {
        expect(() => filtersFromArg('FOO:foobar,BAR:debug'))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('should throw on a string with incorrect formatting', () => {
        expect(() => filtersFromArg('FOO,info,BAR:debug'))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('shoud throw on an invalid argument type', () => {
        expect(() => filtersFromArg(alog.INFO))
          .to.throw().with.property('name', 'AlogConfigError');
      });
    });

    describe('formatterFromArg', () => {
      const formatterFromArg = configure.__get__('formatterFromArg');
      it('should handle a custom function', () => {
        const func = (record: alog.LogRecord) => `RECORD: ${JSON.stringify(record)}`;
        expect(formatterFromArg(func)).to.equal(func);
      });
      it('should handle a valid string from the formatter map', () => {
        expect(formatterFromArg('pretty')).to.equal(alog.PrettyFormatter);
      });
      it('should throw on an invalid string name', () => {
        expect(() => formatterFromArg('foobar'))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('should throw on an invalid type', () => {
        expect(() => formatterFromArg(['pretty']))
          .to.throw().with.property('name', 'AlogConfigError');
      });
    });

    describe('parseConfigureArgs', () => {
      const parseConfigureArgs = configure.__get__('parseConfigureArgs');
      it('should parse a valid object config as argOne', () => {
        const cfg = {defaultLevel: alog.INFO};
        expect(parseConfigureArgs(cfg))
          .to.deep.equal({...cfg, filters: {}, formatter: alog.PrettyFormatter});
      });
      it('should throw on an invalid object config as argOne', () => {
        const cfg = {somethingElse: alog.INFO};
        expect(() => parseConfigureArgs(cfg))
          .to.throw().with.property('name', 'AlogConfigError');
      });
      it('should throw with an object as argOne and filters and formatter passed', () => {
        const cfg = {defaultLevel: alog.INFO};
        expect(() => parseConfigureArgs(cfg, 'FOO:info', 'json'))
          .to.throw().with.property('name', 'AlogConfigError');
      });
    });
  });
});
