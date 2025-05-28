var express = require('express');
var app = express();
var http = require('http').createServer(app);
var io = require('socket.io')(http);
var osc = require('osc-min');
var dgram = require('dgram');
var remote_osc_ip;

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

app.get('/security', function(req, res){
  res.sendFile(__dirname + '/security.html');
});

app.get('/test-encryption', function(req, res){
  res.sendFile(__dirname + '/test-encryption.html');
});

app.use('/public', express.static(__dirname + '/public'));

var udp_server = dgram.createSocket('udp4', function(msg, rinfo) {
  var osc_message;
  try {
    osc_message = osc.fromBuffer(msg);
    console.log(osc_message);
  } catch(err) {
    return console.log('Could not decode OSC message');
  }

  // store the remote ip receiving OSC
  remote_osc_ip = rinfo.address;

  // pipe collected OSC to client's browser via new socket
  io.emit('osc', {
    x: parseInt(osc_message.args[0].value) || 0,
    y: parseInt(osc_message.args[1].value) || 0,
    z: parseInt(osc_message.args[2].value) || 0
  });
  io.emit('internal', {
    x: parseInt(osc_message.args[0].value) || 0,
    y: parseInt(osc_message.args[1].value) || 0,
    z: parseInt(osc_message.args[2].value) || 0
  });

  if (osc_message.address === "/hash") {
    io.emit('hash', {
      value: osc_message.args[0].value,
      timestamp: Date.now()
    });
  }

});

udp_server.bind(9998);
console.log('Listening for OSC messages on port 9998');

http.listen(3000, function(){
  console.log('listening on *:3000');
});