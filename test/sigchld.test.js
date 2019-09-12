var path = require('path')
var hive = require('../lib/hive-cli')
var assert = require('assert')

var _pid
var alive = setInterval(() => {}, 1000)
hive.initHiveProc((pid) => {
  clearInterval(alive)
  assert.strictEqual(pid, _pid)
}).then(() => hive.requestFork(process.cwd(), [path.join(__dirname, 'children', 'echo.js')]))
  .then(pid => { _pid = pid })
