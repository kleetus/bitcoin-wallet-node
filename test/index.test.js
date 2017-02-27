'use strict';
var path = require('path');
var expect = require('chai').expect;
var EventEmitter = require('events');
var exporter = require('../index');

describe('Wallet Exporter', function() {
  var testDirPath = path.resolve(__dirname, './testdata');
  var masterKey = {
    'cipherText':'9167f835344293fcd50aae10d18c84d58f50647fa957b3f16a2baa9e6a11c258415d8c3fa878b3308d92d2da82f52685',
    'derivationMethod':'SHA512',
    'rounds': 239898,
    'salt':'5e7c045cda5034ad',
    'id':1
  };

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
        expect(index).to.equal(2106);
        done();
      });

      exporter.start();
    });

  });

  describe('Pause/Resume', function() {
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
        expect(index).to.equal(2106);
        done();
      });
      resumedExporter.start();
    });
  });
});
