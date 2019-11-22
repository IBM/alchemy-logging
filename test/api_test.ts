
// Standard
import { Writable } from 'stream';

// Third Party
import { expect } from 'chai';
import MemoryStreams from 'memory-streams';
const deepEqual = require('deep-equal');

// Test helpers
import {
  IS_PRESENT,
  sampleLogCode,
  validateLogRecords,
} from './helpers';

// For this test, we only import the public API
const alog = require('rewire')('../src');

/*-- Helpers -----------------------------------------------------------------*/

// This is the ONLY place that we should mess with the internals
function reset() {
  alog.__get__('AlogCoreSingleton').getInstance().reset();
}

/*-- Tests -------------------------------------------------------------------*/

describe('Alog Typescript Public API Test Suite', () => {

  describe('configure', () => {

    beforeEach(() => {
      reset();
    });

    it('should be able to configure with just a default level string', () => {
      alog.configure('debug');
      expect(alog.isEnabled('TEST', alog.DEBUG)).to.be.true;
    });

    it('should be able to configure with just a default level number', () => {
      //DEBUG
    });

    it('should be able to configure with default level string and filter spec string', () => {
      //DEBUG
    });

    it('should be able to configure with default level string and empty filter spec string', () => {
      //DEBUG
    });

    it('should be able to configure with default level number and filter object', () => {
      //DEBUG
    });

    it('should be able to configure with default level number and formatter string', () => {
      //DEBUG
    });

    it('should be able to configure with default level number and formatter function', () => {
      //DEBUG
    });

    it('should be able to configure with config object', () => {
      //DEBUG
    });
  }); // configure

  describe('log functions', () => {
    //DEBUG
  }); // log functions

  describe('indentation', () => {
    //DEBUG
  }); // indentation

  describe('metadata', () => {
    //DEBUG
  }); // metadata

  describe('ChannelLog', () => {
    //DEBUG
  }); // ChannelLog
});
