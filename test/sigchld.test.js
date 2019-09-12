var path = require('path')
var hive = require('../lib/hive-cli')
var assert = require('assert')

var _pid
hive.initHiveProc((pid) => {
  assert.strictEqual(pid, _pid)
  hive.closeHiveProc()
}).then(() => hive.requestFork(process.cwd(), [path.join(__dirname, 'children', 'echo.js')]))
  .then(pid => { _pid = pid })
