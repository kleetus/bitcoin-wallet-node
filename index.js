var Wallet = require('bindings')('wallet');

var ret = Wallet.getMasterKey("/home/k/.bwdb/bitcoin/regtest/wallet.dat");
console.log(ret);
console.log("hi");
