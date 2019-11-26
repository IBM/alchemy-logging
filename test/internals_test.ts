
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
        alogCore.setFormatter(DirectJsonFormatter);
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
        alogCore.setFormatter(DirectJsonFormatter);
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

  // pretty print suite
  describe("PrettyFormatter", () => {

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
  });

  // json suite
  describe("JsonFormatter", () => {

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
