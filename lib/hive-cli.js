'use strict'
var path = require('path')
var Caps = require('@yoda/caps/caps.node').Caps
var hivecli = require('../hivecli.node')

var _childrenPids = []
var _onchld
var _handle

module.exports = {
  initHiveProc: initHiveProc,
  closeHiveProc: closeHiveProc,
  requestFork: requestFork
}

/**
 *
 * @param {string} [hivesock]
 */
function initHiveProc (onchld, hivesock) {
  hivesock = hivesock === undefined ? path.join(process.cwd(), 'hive.sock') : hivesock

  var msg = new Caps()
  msg.writeInt32(/** CommandType::Init */0)
  var packet = msg.serialize()
  var ret = hivecli.request(hivesock, packet)
  if (ret == null) {
    throw new Error('failed')
  }
  if (ret[0] !== 0) {
    return Promise.reject(new Error('failed to init hiveproc'))
  }
  if (process.iotjs) {
    var Pipe = require('pipe_wrap').Pipe
    var handle = new Pipe()
    handle.open(ret.fd)
    handle.onread = onChld
    handle.readStart()
    _handle = handle
  } else {
    var Socket = require('net').Socket
    _handle = new Socket({ fd: ret.fd })
    _handle.on('data', onChld)
  }
  _onchld = onchld

  return Promise.resolve()
}

function closeHiveProc () {
  if (process.iotjs) {
    _handle.close()
  } else {
    _handle.end()
  }
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

  var ret = hivecli.request(hivesock, packet)
  if (ret == null) {
    throw new Error('failed')
  }
  var status = ret[0]
  if (status !== 0) {
    return Promise.reject(new Error('failed to fork'))
  }
  var pid = ret[1]
  if (typeof _onchld === 'function') {
    _childrenPids.push(pid)
  }
  return Promise.resolve(pid)
}

function onChld () {
  if (typeof _onchld !== 'function') {
    return
  }
  _childrenPids = _childrenPids.reduce((accu, it) => {
    try {
      process.kill(it, 0)
      accu.push(it)
    } catch (e) {
      _onchld(it)
    }
    return accu
  }, [])
}
