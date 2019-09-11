var http = require('http')
var req = http.request({
  host: '127.0.0.1',
  port: Number(process.argv[2]),
  headers: { PID: String(process.pid) }
}, () => {})
req.write('d')
req.end()
