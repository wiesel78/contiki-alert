
var mqtt = require('mqtt');
var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);

var devices = {};

app.use(express.static('public/'));

http.listen(8080, function () {
  console.log('Example app listening on port 8080!');
});


io.on('connection', function(socket){
  console.log('socket.io user connected', devices);
  io.emit("SaveDevices", devices);
});


var client = mqtt.connect('mqtt://localhost:1883');

client.on('connect', () => {
    client.subscribe('clients/+')
})

client.on('message', (topic, message) => {
    var msg = JSON.parse(message.toString());

    console.log(topic, " : ", msg);

    if(topic.match(/^clients\/.*/))
    {
        console.log("id : ", msg.id);
        devices[msg.id] = msg;
        io.emit("SaveDevices", devices);
    }

})
