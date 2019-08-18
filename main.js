var express = require('express');
var app = express();
var http = require('http').createServer(app);
var io = require('socket.io')(http);
var osc = require('osc-min'),
    dgram = require('dgram'),
    remote_osc_ip;

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

app.use('/public', express.static(__dirname + '/public'));
 
var udp_server = dgram.createSocket('udp4', function(msg, rinfo) {

  var osc_message;
  try {
    osc_message = osc.fromBuffer(msg);
    //console.log(osc_message);
  } catch(err) {
    return console.log('Could not decode OSC message');
  }

  remote_osc_ip = rinfo.address; 

  io.emit('osc', {
    x: parseInt(osc_message.args[0].value) || 0,
    y: parseInt(osc_message.args[1].value) || 0,
    z: parseInt(osc_message.args[2].value) || 0
  });

});

//LOADERS

udp_server.bind(9998);
console.log('Listening for OSC messages on port 9998');

http.listen(3000, function(){
  console.log('listening on *:3000');
});