var path = require('path')
var http = require('http')
var assert = require('assert')
var hive = require('../lib/hive-cli')

var forkPid
var reqPid
process.on('exit', () => {
  assert.strictEqual(forkPid, reqPid)
})

hive.initHiveProc()
  .then(run)
  .then(hive.closeHiveProc)

function run () {
  return new Promise(resolve => {
    var server = http.createServer((req, res) => {
      console.log('incoming req', req.headers)
      reqPid = Number(req.headers.pid)
      var bufs = []
      req.on('data', data => {
        bufs.push(data)
      })
      req.on('end', () => {
        res.statusCode = 200
        res.end()
        server.close()
        resolve()
      })
    })
    server.listen(0, () => {
      var port = server.address().port
      hive.requestFork(process.cwd(), [path.join(__dirname, 'children', 'http-cli.js'), String(port)])
        .then(pid => {
          forkPid = pid
          console.log('forked child', pid)
        })
    })
  })
}
