'use strict';
var path = require('path');
var expect = require('chai').expect;
var EventEmitter = require('events');
var exporter = require('../index');

describe('Wallet Exporter', function() {
  var testDirPath = path.resolve(__dirname, './testdata');

  describe('Unencrypted Modern Wallets', function() {
    var index = 0;
    var emitter = new EventEmitter();
    it('stream out no keys', function(done) {
      exporter.stream({
        filePath: testDirPath + '/wallet_unencrypted.dat',
        emitter: emitter
      });

      emitter.on('data', function(value) {
        index++;
      });

      emitter.on('close', function() {
        expect(index).to.equal(0);
        done();
      });

      exporter.start();
    });

  });

  describe('Encrypted Modern Wallets', function() {
    var index = 0;
    var masterKey = {
      'cipherText':'1ab1faacfb9eb8017f9c180aeab425ca9c6e9248deae186c' +
                   '0d0260044e607519b443fd97fee8229070e9b3e25382cb1a',
      'derivationMethod':'SHA512',
      'rounds':166666,
      'salt':'867ddcee1da2143b',
      'id':1
    };
    var emitter = new EventEmitter();
    it('stream out keys', function(done) {

      exporter.stream({
        filePath: testDirPath + '/wallet_encrypted.dat',
        emitter: emitter
      });

      emitter.on('data', function(value) {
        if (index === 0) {
          expect(JSON.parse(value)).to.deep.equal(masterKey);
        } else {
          expect(Object.keys(JSON.parse(value))).to.deep.equal(['cipherText','pubKey']);
        }
        index++;
      });

      emitter.on('close', function() {
        expect(index).to.equal(203);
        done();
      });

      exporter.start();
    });

  });

  describe('Pause/Resume', function() {
    var masterKey = {
      'cipherText':'1ab1faacfb9eb8017f9c180aeab425ca9c6e9248deae186c' +
        '0d0260044e607519b443fd97fee8229070e9b3e25382cb1a',
      'derivationMethod':'SHA512',
      'rounds':166666,
      'salt':'867ddcee1da2143b',
      'id':1
    };
    it('paused', function(done) {
      var emitter = new EventEmitter();
      var pausedExporter = require('../index');
      var index = 0;
      pausedExporter.stream({
        filePath: testDirPath + '/wallet_encrypted.dat',
        emitter: emitter
      });
      pausedExporter.pause();
      emitter.on('data', function(value) {
        index++;
        expect(JSON.parse(value)).to.deep.equal(masterKey);
        exporter.close();
      });
      emitter.on('close', function() {
        expect(index).to.equal(1);
        done();
      });
      pausedExporter.start();
    });
    it('resumed', function(done) {
      this.timeout(10000);
      var emitter = new EventEmitter();
      var resumedExporter = require('../index');
      var index = 0;
      resumedExporter.stream({
        filePath: testDirPath + '/wallet_encrypted.dat',
        emitter: emitter
      });
      resumedExporter.pause();
      emitter.on('data', function(value) {
        index++;
        setTimeout(function() {
          expect(JSON.parse(value)).to.deep.equal(masterKey);
          expect(index).to.equal(1);
          resumedExporter.resume();
        }, 1000);
      });
      emitter.on('close', function() {
        expect(index).to.equal(203);
        done();
      });
      resumedExporter.start();
    });
  });
});
