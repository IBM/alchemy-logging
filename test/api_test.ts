
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
  sampleLogCode,
  stubValidationRecord,
  validateLogRecords,
} from './helpers';

// For this test, we are testing the public API. As such, we only want a couple
// of internals available for validation.
const alog = require('rewire')('../src');
const levelFromName = alog.__get__('levelFromName');
const nameFromLevel = alog.__get__('nameFromLevel');

/*-- Tests -------------------------------------------------------------------*/

describe('Alog Typescript Public API Test Suite', () => {

  beforeEach(() => {
    alog.__get__('AlogCoreSingleton').getInstance().reset();
  });

  describe('configure', () => {

    it('should be able to configure with just a default level string', () => {
      alog.configure('debug');
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
    });

    it('should be able to configure with just a default level number', () => {
      alog.configure(alog.DEBUG);
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
    });

    it('should be able to configure with default level string and filter spec string', () => {
      alog.configure('debug', 'FOO:info');
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
      expect(alog.isEnabled('FOO', alog.DEBUG)).to.be.false;
    });

    it('should be able to configure with default level string and empty filter spec string', () => {
      alog.configure('debug', '');
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
      expect(alog.isEnabled('FOO', alog.DEBUG)).to.be.true;
    });

    it('should be able to configure with default level number and filter object', () => {
      alog.configure(alog.DEBUG, {FOO: alog.INFO});
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
      expect(alog.isEnabled('FOO', alog.DEBUG)).to.be.false;
    });

    it('should be able to configure with default level number and formatter string', () => {
      alog.configure(alog.DEBUG, '', 'json');
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
      expect(alog.isEnabled('FOO', alog.DEBUG)).to.be.true;
    });

    it('should be able to configure with default level number and formatter function', () => {
      let loggedIt: boolean = false;
      alog.configure(alog.DEBUG, '', () => { loggedIt =  true; });
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
      expect(alog.isEnabled('FOO', alog.DEBUG)).to.be.true;
      alog.debug('TEST', 'This is a test');
      expect(loggedIt).to.be.true;
    });

    it('should be able to configure with config object', () => {
      alog.configure({
        defaultLevel: alog.DEBUG,
        filters: {
          FOO: alog.INFO,
        },
      });
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
      expect(alog.isEnabled('FOO', alog.DEBUG)).to.be.false;
    });
  }); // configure

  describe('log functions', () => {

    let logStream: Writable;
    beforeEach(() => {
      alog.configure({
        defaultLevel: alog.DEBUG,
        filters: {
          LOWER: alog.WARNING,
          HIGHER: alog.DEBUG2,
        },
        formatter: DirectJsonFormatter,
      });
      logStream = new MemoryStreams.WritableStream();
      alog.addOutputStream(logStream);
    });

    it('should have all the expected level functions', () => {
      for (const levelName of Object.keys(levelFromName)) {
        expect(alog).to.have.property(levelName);
      }
    });

    it('should log with a level-function when enabled by the default level', () => {
      alog.debug('TEST', "Some fun message");
      expect(validateLogRecords(getLogRecords(logStream), [
        {
          channel: 'TEST', level: alog.DEBUG, level_str: nameFromLevel[alog.DEBUG],
          timestamp: IS_PRESENT, num_indent: 0,
          message: "Some fun message",
        },
      ])).to.be.true;
    });

    it('should not log with a level-function when disabled by the default level', () => {
      alog.debug3('TEST', "Some fun message");
      expect(validateLogRecords(getLogRecords(logStream), [])).to.be.true;
    });

    it('should log with a level-function when enabled by the filters', () => {
      alog.debug2('HIGHER', "Some fun message");
      expect(validateLogRecords(getLogRecords(logStream), [
        {
          channel: 'HIGHER', level: alog.DEBUG2, level_str: nameFromLevel[alog.DEBUG2],
          timestamp: IS_PRESENT, num_indent: 0,
          message: "Some fun message",
        },
      ])).to.be.true;
    });

    it('should not log with a level-function when disabled by the filters', () => {
      alog.info('LOWER', "Some fun message");
      expect(validateLogRecords(getLogRecords(logStream), [])).to.be.true;
    });
  }); // log functions

  describe('indentation', () => {

    let logStream: Writable;
    beforeEach(() => {
      alog.configure(alog.DEBUG, null, DirectJsonFormatter);
      logStream = new MemoryStreams.WritableStream();
      alog.addOutputStream(logStream);
    });

    it('should correctly indent and deindent multiple times', () => {
      alog.debug('TEST', "Some fun message");
      alog.indent();
      alog.debug('TEST', "One level in");
      alog.indent();
      alog.debug('TEST', "Two levels in");
      alog.deindent();
      alog.debug('TEST', "Back to one level");
      alog.deindent();
      alog.debug('TEST', "All the way back");
      expect(validateLogRecords(getLogRecords(logStream), [
        Object.assign(stubValidationRecord(), {num_indent: 0}),
        Object.assign(stubValidationRecord(), {num_indent: 1}),
        Object.assign(stubValidationRecord(), {num_indent: 2}),
        Object.assign(stubValidationRecord(), {num_indent: 1}),
        Object.assign(stubValidationRecord(), {num_indent: 0}),
      ])).to.be.true;
    });

    /*
    // SCOPED INDENT TESTS
    */
  }); // indentation

  describe('metadata', () => {

    let logStream: Writable;
    beforeEach(() => {
      alog.configure(alog.DEBUG, undefined, DirectJsonFormatter);
      logStream = new MemoryStreams.WritableStream();
      alog.addOutputStream(logStream);
    });

    it('should add and remove metadata correctly', () => {
      alog.info('TEST', 'A');
      alog.addMetadata('foo', 1);
      alog.info('TEST', 'B');
      alog.removeMetadata('foo');
      alog.info('TEST', 'C');
      expect(validateLogRecords(getLogRecords(logStream), [
        Object.assign(stubValidationRecord(), {message: 'A'}),
        Object.assign(stubValidationRecord(), {message: 'B', metadata: {foo: 1}}),
        Object.assign(stubValidationRecord(), {message: 'C'}),
      ])).to.be.true;
    });

    /*
    // SCOPED METADATA TESTS
    */

  }); // metadata

  describe('ChannelLog', () => {
    /*
    // CHANNEL LOG TESTS
    */
  }); // ChannelLog
});
