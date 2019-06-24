require('./pre-load')

var flora = require('@yoda/flora')
var logger = require('logger')('hive')
var hive = require('hive')

main()

function main () {
  var agent = new flora.Agent('unix:/var/run/flora.sock#hive')
  var runtimePid = hive.fork('/', '/usr/yoda/services/vuid/index.js', process.env)
  if (runtimePid === 0) {
    return
  }
  logger.info(`forked runtime(${runtimePid})`)
  hive.onExit((pid, code, status) => {
    if (pid === runtimePid) {
      return process.exit(127)
    }
    agent.post('yodaos.hive.on-child-exit', [ pid, code, status ])
  })

  agent.declareMethod('yodaos.hive.incubate', (reqMsg, res, sender) => {
    var cwd = reqMsg[0]
    var argv = reqMsg[1]
    var environs = {}
    var environArr = reqMsg[3]
    if (Array.isArray(environArr) && environArr.length % 2 === 0) {
      for (var idx = 0; idx < environArr.length; idx += 2) {
        environs[environArr[idx]] = environArr[idx + 1]
      }
    }

    var pid = hive.fork(cwd, argv, environs)
    if (pid === 0) {
      logger.info(`child forked`)
      /**
       * FIXME: agent.close() is not idempotent and would throw on child process.
       * Using unref for exiting child process properly.
       */
      return
    }
    logger.info(`forked child(${pid})`)
    if (pid < 0) {
      res.end(1, [ pid ])
      return
    }
    res.end(0, [ pid ])
  })
  agent.start()
}
