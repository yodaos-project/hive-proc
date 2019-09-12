'use strict'
var net = require('net')
var path = require('path')
var Caps = require('@yoda/caps/caps.node').Caps

var _childrenPids = []
var _onexit

module.exports = {
  initHiveProc: initHiveProc,
  requestFork: requestFork
}

/**
 *
 * @param {string} [hivesock]
 */
function initHiveProc (onexit, hivesock) {
  hivesock = hivesock === undefined ? path.join(process.cwd(), 'hive.sock') : hivesock

  return new Promise((resolve, reject) => {
    var conn = net.createConnection(hivesock)
    var msg = new Caps()
    msg.writeInt32(/** CommandType::Init */0)
    var packet = msg.serialize()

    var bufs = []
    conn.on('data', buf => {
      bufs.push(buf)
    })
    conn.on('end', () => {
      var buf = Buffer.concat(bufs)
      var caps = new Caps()
      caps.deserialize(buf)
      if (caps.readInt32() !== 0) {
        reject(new Error('failed to init hiveproc'))
        return
      }
      _onexit = onexit
      process.on('SIGCHLD', onSigchld)
      resolve()
    })
    conn.write(packet)
    conn.end()
  })
}

/**
 *
 * @param {string} cwd
 * @param {string[]} args
 * @param {{ [key: string]: string }} environs
 * @param {string} [hivesock]
 */
function requestFork (cwd, args, environs, hivesock) {
  cwd = cwd === undefined ? process.cwd() : cwd
  args = args === undefined ? [] : args
  environs = environs == null ? {} : environs
  hivesock = hivesock === undefined ? path.join(process.cwd(), 'hive.sock') : hivesock

  return new Promise((resolve, reject) => {
    var conn = net.createConnection(hivesock)
    var msg = new Caps()
    msg.writeInt32(/** CommandType::Fork */1)
    msg.writeString(/** cwd */cwd)
    var argsCaps = new Caps()
    args.forEach(it => {
      argsCaps.writeString(it)
    })
    msg.writeCaps(argsCaps)
    var environsCaps = new Caps()
    Object.keys(environs).forEach(key => {
      environsCaps.writeString(key)
      environsCaps.writeString(String(environs[key]))
    })
    msg.writeCaps(environsCaps)
    var packet = msg.serialize()

    var bufs = []
    conn.on('data', buf => {
      bufs.push(buf)
    })
    conn.on('end', () => {
      var buf = Buffer.concat(bufs)
      var caps = new Caps()
      caps.deserialize(buf)
      var status = caps.readInt32()
      if (status !== 0) {
        reject(new Error('failed to fork'))
        return
      }
      var pid = caps.readInt32()
      _childrenPids.push(pid)
      resolve(pid)
    })
    conn.write(packet)
    conn.end()
  })
}

function onSigchld () {
  if (typeof _onexit !== 'function') {
    return
  }
  _childrenPids.forEach(it => {
    try {
      process.kill(it, 0)
    } catch (e) {
      _onexit(it)
    }
  })
}
