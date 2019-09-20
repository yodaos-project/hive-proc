var path = require('path')
var hivecli = require('../hivecli.node')
var assert = require('assert')
var Caps = require('@yoda/caps/caps.node').Caps

console.log(process.pid)

var msg = new Caps()
msg.writeString('foo')
var packet = msg.serialize()
console.log('Binary length:', packet.byteLength)

var ret = hivecli.request(path.join(process.cwd(), 'echo.sock'), packet)
assert(Array.isArray(ret), 'expect an array')
assert(ret[0] === 'foo', 'expect echo result')
