###Purpose
To export/extract keys from a Bitcoin wallet.dat file. More specifically, streams out keys from a Bitcoin Core's (formerly known as Sastoshi Client) wallet file (wallet.dat).

###Dependencies/Prerequisties
* C++ compiler and build tools
* Mac/Linux (Windows not yet supported)
* node version >= 4.x

###Installation
```bash
$ npm install
```

###Usage
```javascript
var exporter = require('bitcoin-wallet-node');

exporter({ filePath: './wallet.dat' }); //to stdout -or- send in your own emitter, just needs to be able to emit a data, close and error signal

var EventEmitter = require('events');

var emitter = new EventEmitter();

exporter({ filePath: './wallet.dat', emitter: emitter }); //send in an emitter
emitter.on('data', function(value) {
  console.log(value);
});
exporter.start(); //this allows the db to be opened, read, and sent out record by record
export.close(); //this will finish sending the record in progress, then close the stream out for good
```

To pause/resume the exporter
```bash
exporter.pause();
exporter.resume();
```
###Implementation notes
The process doing the reading of the database is on a separate thread from the main node process (the one running your script).
We can't rely on the normal reactor pattern coding when it comes to signaling the worker process. The worker process doesn't know anything about setImmediate or nextTick, etc.. This means that a start function must be explicitly called if we are going to have any chance predicting when a subsequent pause signal might be considered by the worker thread. You may call pause() before start() for the purposes of emitting one record before pausing. Calling pause() after start() will ensure that will get at least one more record (but maybe 2 or 3) before pausing. Calling resume() will pick up where things left off.

###Common issues
* If you get an error/exception about crypto support not available, then please read the following:
  * This native addon includes Berkeley DB source code compatible with Bitcoin's wallet.dat file, but it does not come with cryptography support. Government export restrictions are to blame.
  * Wallet.dat files used in older versions of Bitcoin used Berkeley DB-based crypto (such aes-cbc routines). Newer Bitcoin wallet.dat files use crypto routines provided by Openssl or Bitcoin's source code directly. So if you have an older wallet.dat file needing crypto support in Berkeley DB, then link this library with your system's libdb_cxx-4.8.{a|so|dylib}. Use the bindings.gyp.alt file for this purpose. Just rename "bindings.gyp.alt" to "bindings.gyp", install Berkeley DB 4.8.30 from brew/apt/dnf/yum/pacman, then npm install. You will also need a single header file, db_cxx.h. AFAIK, you can use the one provided in this project because it has stubs crypto routines, but please make sure the Berkeley DB library version is 4.8.30. If you end up installed 4.8.30 shared/static library from Oracle's source or from your OS repo, then just ensure this header is in a standard location and remove the following line from bindings.gyp: "./dependencies/db-4.8.30.NC/build_unix",

###Notes
* This module does not handle unencrypted keys (e.g. wallet.dat files that are not encrypted). It is incredibly dangerous to store a wallet file unencrypted. Please always encrypt your wallet.dat file in Bitcoin.
* The "master" key is always streamed out first. This allows consumers of the output to decrypt the master key with a passphrase, then decrypt each key, if desired, for verification or construction of a new transaction.
