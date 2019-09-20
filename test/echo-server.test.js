var path = require('path')
var hivecli = require('../hivecli.node')
var assert = require('assert')
var childProcess = require('child_process')
var Caps = require('@yoda/caps/caps.node').Caps

var sockpath = path.join(process.cwd(), 'echo.sock')
console.log('+', `${path.resolve(__dirname, '../out/build/hiveproc/echo-server')} ${sockpath}`)
var cp = childProcess.spawn(path.resolve(__dirname, '../out/build/hiveproc/echo-server'), [sockpath], { stdio: 'inherit' })
cp.on('error', console.error)
cp.on('exit', () => console.log('echo server exited'))

setTimeout(main, 1000)

function main () {
  var msg = new Caps()
  msg.writeString('foo')
  var packet = msg.serialize()
  console.log('Binary length:', packet.byteLength)

  var ret = hivecli.request(sockpath, packet)
  assert(Array.isArray(ret), 'expect an array')
  assert(ret[0] === 'foo', 'expect echo result')
  cp.kill()
}
