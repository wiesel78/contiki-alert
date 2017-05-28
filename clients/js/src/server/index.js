
var mqtt = require('mqtt');
var express = require('express');
var app = express();

app.use(express.static('src/client/'));

app.listen(8080, function () {
  console.log('Example app listening on port 8080!');
});


var client = mqtt.connect('mqtt://localhost:1883');

client.on('connect', () => {
  client.subscribe('clients/+')
})

client.on('message', (topic, message) => {
  console.log(topic, " : ", message.toString());
})
