var net = require('net')
var path = require('path')
var Caps = require('@yoda/caps/caps.node').Caps
var http = require('http')
var assert = require('assert')

var forkPid
var reqPid
var server = http.createServer((req, res) => {
  console.log('incoming req', req.headers)
  reqPid = Number(req.headers.pid)
  req.on('close', () => {
    console.log('req end')
    res.end()
    server.close()
  })
})
server.listen(0, () => {
  var port = server.address().port
  requestFork([path.join(__dirname, 'children', 'http-cli.js'), String(port)])
    .then(pid => {
      forkPid = pid
      console.log('forked child', pid)
    })
})
process.on('exit', () => {
  assert.strictEqual(forkPid, reqPid)
})

function requestFork (args) {
  return new Promise(resolve => {
    var conn = net.createConnection(path.join(process.cwd(), 'hive.sock'))
    var msg = new Caps()
    var argv = new Caps()
    args.forEach(it => {
      argv.writeString(it)
    })
    msg.writeCaps(argv)
    var environs = new Caps()
    msg.writeCaps(environs)
    var packet = msg.serialize()

    var bufs = []
    conn.on('data', buf => {
      console.log('received', buf.byteLength)
      bufs.push(buf)
    })
    conn.on('end', () => {
      console.log('end', bufs.length)
      var buf = Buffer.concat(bufs)
      var caps = new Caps()
      caps.deserialize(buf)
      var pid = caps.readInt32()
      console.log('received', pid)
      resolve(pid)
    })
    conn.write(packet)
    conn.end()
  })
}
