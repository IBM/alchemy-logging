
// Standard
import { Writable } from 'stream';

// Third Party
import { expect } from 'chai';
import MemoryStreams from 'memory-streams';
const deepEqual = require('deep-equal');

// Test helpers
import {
  getLogRecords,
  IS_PRESENT,
  sampleLogCode,
  validateLogRecords,
} from './helpers';

// Things under test (will be monkey patched)
const rewire = require('rewire');
const alog  = rewire('../src');
const isLogCode = alog.__get__('isLogCode');
const AlogCoreSingleton = alog.__get__('AlogCoreSingleton');
const PrettyFormatter = alog.__get__('PrettyFormatter');
const JsonFormatter = alog.__get__('JsonFormatter');
const levelFromName = alog.__get__('levelFromName');
const nameFromLevel = alog.__get__('nameFromLevel');

/*-- Tests -------------------------------------------------------------------*/

describe("Alog TypeScript Internals Test Suite", () => {
  // core singleton suite
  describe("AlogCoreSingleton", () => {

    describe("mutators", () => {
      const alogCore = AlogCoreSingleton.getInstance();
      beforeEach(() => {
        alogCore.reset();
      });

      it("should be able to set the default level", () => {
        expect((alogCore as any).defaultLevel).to.equal(alog.OFF);
        alogCore.setDefaultLevel(alog.DEBUG);
        expect((alogCore as any).defaultLevel).to.equal(alog.DEBUG);
      });

      it("should be able to set filters", () => {
        expect((alogCore as any).defaultLevel).to.equal(alog.OFF);
        alogCore.setFilters({TEST: alog.INFO});
        expect((alogCore as any).filters).to.deep.equal({TEST: alog.INFO});
      });

      it("should be able to set the formatter", () => {
        expect((alogCore as any).formatter.name).to.equal(PrettyFormatter.name);
        alogCore.setFormatter(JsonFormatter);
        expect((alogCore as any).formatter.name).to.equal(JsonFormatter.name);
      });

      it("should be able to indent", () => {
        expect((alogCore as any).numIndent).to.equal(0);
        alogCore.indent();
        expect((alogCore as any).numIndent).to.equal(1);
      });

      it("should be able to deindent", () => {
        expect((alogCore as any).numIndent).to.equal(0);
        alogCore.indent();
        expect((alogCore as any).numIndent).to.equal(1);
        alogCore.deindent();
        expect((alogCore as any).numIndent).to.equal(0);
      });

      it("should be not able to deindent past 0", () => {
        expect((alogCore as any).numIndent).to.equal(0);
        alogCore.deindent();
        expect((alogCore as any).numIndent).to.equal(0);
      });

      it("should be able to add metadata", () => {
        expect((alogCore as any).metadata).to.deep.equal({});
        alogCore.addMetadata('key', {nested: 1});
        expect((alogCore as any).metadata).to.deep.equal({key: {nested: 1}});
      });

      it("should be able to remove metadata", () => {
        expect((alogCore as any).metadata).to.deep.equal({});
        alogCore.addMetadata('key', {nested: 1});
        expect((alogCore as any).metadata).to.deep.equal({key: {nested: 1}});
        alogCore.removeMetadata('key');
        expect((alogCore as any).metadata).to.deep.equal({});
      });

      it("should ignore request to remove unknown metadata", () => {
        expect((alogCore as any).metadata).to.deep.equal({});
        alogCore.addMetadata('key', {nested: 1});
        expect((alogCore as any).metadata).to.deep.equal({key: {nested: 1}});
        alogCore.removeMetadata('foobar');
        expect((alogCore as any).metadata).to.deep.equal({key: {nested: 1}});
      });

      it("should be able to add a custom stream", () => {
        expect((alogCore as any).streams.length).to.equal(1);
        alogCore.addOutputStream(new MemoryStreams.WritableStream());
        expect((alogCore as any).streams.length).to.equal(2);
      });

      it("should be able to reset output streams", () => {
        expect((alogCore as any).streams.length).to.equal(1);
        alogCore.addOutputStream(new MemoryStreams.WritableStream());
        expect((alogCore as any).streams.length).to.equal(2);
        alogCore.resetOutputStreams();
        expect((alogCore as any).streams.length).to.equal(1);
      });
    }); // mutators

    describe("isEnabled", () => {
      const alogCore = AlogCoreSingleton.getInstance();
      beforeEach(() => {
        alogCore.reset();
        alogCore.setDefaultLevel(alog.DEBUG);
        alogCore.setFilters({LOWER: alog.INFO, HIGHER: alog.DEBUG2});
      });

      it("should return enabled false using the default level", () => {
        expect(alogCore.isEnabled('TEST', alog.DEBUG4)).to.be.false;
      });

      it("should return enabled true using the default level", () => {
        expect(alogCore.isEnabled('TEST', alog.DEBUG)).to.be.true;
      });

      it("should return enabled false using a filter lower than the default", () => {
        expect(alogCore.isEnabled('LOWER', alog.DEBUG)).to.be.false;
      });

      it("should return enabled true using a filter lower than the default", () => {
        expect(alogCore.isEnabled('LOWER', alog.WARNING)).to.be.true;
      });

      it("should return enabled false using a filter higher than the default", () => {
        expect(alogCore.isEnabled('HIGHER', alog.DEBUG3)).to.be.false;
      });

      it("should return enabled true using a filter higher than the default", () => {
        expect(alogCore.isEnabled('HIGHER', alog.DEBUG1)).to.be.true;
      });
    }); // isEnabled

    describe("log", () => {

      const alogCore = AlogCoreSingleton.getInstance();
      let logStream: Writable;
      beforeEach(() => {
        alogCore.reset();
        alogCore.setDefaultLevel(alog.DEBUG);
        alogCore.setFilters({LOWER: alog.INFO, HIGHER: alog.DEBUG2});
        alogCore.setFormatter(JsonFormatter);
        logStream = new MemoryStreams.WritableStream();
        alogCore.addOutputStream(logStream);
      });

      it("should be able to log with signature 1", () => {
        alogCore.log(alog.DEBUG, 'TEST', sampleLogCode, () => "This is a generated message", {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', sampleLogCode, () => "This is a second generated message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is a generated message",
            metadata: {foo: 'bar'},
            log_code: sampleLogCode,
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is a second generated message",
            log_code: sampleLogCode,
          },
        ])).to.be.true;
      });

      it("should be able to log with signature 2", () => {
        alogCore.log(alog.DEBUG, 'TEST', sampleLogCode, "This is NOT a generated message", {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', sampleLogCode, "This is NOT a second generated message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is NOT a generated message",
            metadata: {foo: 'bar'},
            log_code: sampleLogCode,
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is NOT a second generated message",
            log_code: sampleLogCode,
          },
        ])).to.be.true;
      });

      it("should be able to log with signature 3", () => {
        alogCore.log(alog.DEBUG, 'TEST', () => "This is a generated message", {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', () => "This is a second generated message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is a generated message",
            metadata: {foo: 'bar'},
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is a second generated message",
          },
        ])).to.be.true;
      });

      it("should be able to log with signature 4", () => {
        alogCore.log(alog.DEBUG, 'TEST', "This is NOT a generated message", {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', "This is NOT a second generated message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is NOT a generated message",
            metadata: {foo: 'bar'},
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is NOT a second generated message",
          },
        ])).to.be.true;
      });

      it("should correctly log merged global and local metadata", () => {
        alogCore.addMetadata('baz', 1);
        alogCore.log(alog.DEBUG, 'TEST', "This is NOT a generated message", {foo: 'bar'});
        alogCore.log(alog.INFO, 'FOO', "This is NOT a second generated message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is NOT a generated message",
            metadata: {foo: 'bar', baz: 1},
          },
          {
            channel: 'FOO', level: alog.INFO, level_str: nameFromLevel[alog.INFO],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "This is NOT a second generated message",
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
        alogCore.setFormatter(JsonFormatter);
        logStream = new MemoryStreams.WritableStream();
        alogCore.addOutputStream(logStream);
      });

      it('should have all the expected level functions', () => {
        for (const levelName of Object.keys(levelFromName)) {
          expect(alogCore).to.have.property(levelName);
        }
      });

      it('should log with a level-function when enabled by the default level', () => {
        alogCore.debug('TEST', "Some fun message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "Some fun message",
          },
        ])).to.be.true;
      });

      it('should not log with a level-function when disabled by the default level', () => {
        alogCore.debug3('TEST', "Some fun message");
        expect(validateLogRecords(getLogRecords(logStream), [])).to.be.true;
      });

      it('should log with a level-function when enabled by the filters', () => {
        alogCore.debug2('HIGHER', "Some fun message");
        expect(validateLogRecords(getLogRecords(logStream), [
          {
            channel: 'HIGHER', level: alog.DEBUG2, level_str: nameFromLevel[alog.DEBUG2],
            timestamp: IS_PRESENT, num_indent: 0,
            message: "Some fun message",
          },
        ])).to.be.true;
      });

      it('should not log with a level-function when disabled by the filters', () => {
        alogCore.info('LOWER', "Some fun message");
        expect(validateLogRecords(getLogRecords(logStream), [])).to.be.true;
      });
    });

  }); // AlogCoreSingleton

  // configure suite

  // pretty print suite

  // json suite

  // input validation suite
  describe("Input validation suite", () => {
    // Checks for verifying that log codes are valid or invalid
    describe("Log Code Validation", () => {
      it("A happy log code is valid", () => {
        expect(isLogCode("<ORC12345678D>")).to.be.true;
      });

      it("A log code that is missing its starting angle bracket should fail", () => {
        expect(isLogCode("ORC12345678D>")).to.be.false;
      });

      it("A log code that is missing its closing angle bracket should fail", () => {
        expect(isLogCode("<ORC12345678D")).to.be.false;
      });

      it("A log code that has more than 8 digits should fail", () => {
        expect(isLogCode("<ORC1234544242678D>")).to.be.false;
      });

      it("A log code that has less than 8 digits should fail", () => {
        expect(isLogCode("<ORC178D>")).to.be.false;
      });

      it("A log code that has lowercase letters should fail", () => {
        expect(isLogCode("<orc12345678D>")).to.be.false;
      });

      it("A log code that has a lowercase letter for its level key should fail", () => {
        expect(isLogCode("<ORC12345678d>")).to.be.false;
      });

      it("A log code that is missing a level key should fail", () => {
        expect(isLogCode("<ORC12345678>")).to.be.false;
      });

      it("A log code that has a level key that is not in {IWTDEF} should fail", () => {
        expect(isLogCode("<ORC12345678Z>")).to.be.false;
      });
    });
  });

  // child logger stuff suite
});
