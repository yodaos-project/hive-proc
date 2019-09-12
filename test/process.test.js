var path = require('path')
var http = require('http')
var assert = require('assert')
var hive = require('../lib/hive-cli')

hive.initHiveProc()
  .then(run)
  .then(hive.closeHiveProc)

function run () {
  return new Promise(resolve => {
    var server = http.createServer((req, res) => {
      console.log('incoming req', req.headers)
      var bufs = []
      req.on('data', data => {
        bufs.push(data)
      })
      req.on('end', () => {
        res.statusCode = 200
        res.end()

        var buf = Buffer.concat(bufs).toString()
        var data
        try { data = JSON.parse(buf) } catch (err) { assert.fail('unable parse data') }
        assert.strictEqual(data.argv[1], path.join(__dirname, 'children', 'http-cli.js'))

        server.close()
        resolve()
      })
    })
    server.listen(0, () => {
      var port = server.address().port
      hive.requestFork(process.cwd(), [path.join(__dirname, 'children', 'http-cli.js'), String(port)])
        .then(pid => {
          console.log('forked child', pid)
        })
    })
  })
}
