var http = require('http')

var data = Buffer.from(JSON.stringify({ argv: process.argv, env: process.env }))
var req = http.request({
  host: '127.0.0.1',
  port: Number(process.argv[2]),
  headers: { pid: String(process.pid), 'content-type': 'application/json', 'content-length': data.byteLength }
}, () => {})
req.write(data, (err) => { console.log('written', err); req.end() })
