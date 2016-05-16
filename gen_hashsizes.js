"use strict";

var out = ""
var max = 32*1024*1024;

var buf = new Int32Array(max);

for (var i=0; i<max; i++) {
  buf[i] = i;
}

for (var i=2; i<max; i++) {
  for (var j=i*2; j<max; j += i) {
    buf[j] = -1;
  }
}

var primes = []
var last = 1;
for (var i=2; i<max; i++) {
  if (buf[i] != -1 && buf[i] > last*3.35) {
    primes.push(i);
    last = i;
  }
}

console.log(primes);