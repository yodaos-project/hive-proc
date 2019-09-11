require(process.argv[2])
var hiveproc = require('../hiveproc.node')
main()

function main () {
  var pid = hiveproc.forkAndSpecialize(process)
  if (pid < 0) {
    // TODO: fatal error
    return process.exit(1)
  }
  require(process.argv[1])
}
