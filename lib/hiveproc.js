require(process.argv[2])
var hiveproc = require('../hiveproc.node')
main(process.argv[3])

function main (entry) {
  var pid = hiveproc.forkAndSpecialize(process, entry)
  if (pid < 0) {
    return process.exit(1)
  }
  require(process.argv[1])
}
