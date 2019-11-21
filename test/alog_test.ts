import { expect } from 'chai';

// Things under test (will be monkey patched)
const rewire = require('rewire');
const alog  = rewire('../src');
const isLogCode = alog.__get__("isLogCode");

describe("Alog TypeScript Test Suite", () => {
  // custom stream suite
  describe("Custom Stream Test Suite", () => {
    it("Should attach an output string stream", () => {
      //DEBUG
    });
  });
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
