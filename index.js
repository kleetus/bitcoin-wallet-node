var wallet = require('./build/Release/wallet');
var stream;

exports.stream = function(opts) {
  var emitter;
  if (opts.emitter) {
    emitter = opts.emitter;
  } else {
    var EventEmitter = require('events');
    emitter = new EventEmitter();
  }
  stream = new wallet.StreamingWorker(function(event, value) {
    emitter.emit('data', value);
  }, function() {
    emitter.emit('close');
  }, function(error) {
    emitter.emit('error', error);
  }, opts);
  if (!opts.emitter) {
    emitter.on('data', function(value) {
      process.stdout.write(value + '\n');
    });
  }
};

exports.start = function() {
  if (stream) {
    stream.start();
  }
};

exports.pause = function() {
  if (stream) {
    stream.pause();
  }
};

exports.resume = function() {
  if (stream) {
    stream.resume();
  }
};

exports.close = function() {
  if (stream) {
    stream.closeInput();
  }
};
