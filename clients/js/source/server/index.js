
var mqtt = require('mqtt');
var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var bodyParser = require('body-parser');

// iot alert api over mqtt
var mqttAlert = require('./mqtt-alert');

// SOCKET-IO
io.on('connection', function(socket){
  console.log('socket.io user connected');
  io.emit("SaveDevices", mqttAlert.getDevices());
});

// Alert bindings ans connect to devices
mqttAlert.connect('mqtt://localhost:1883');
mqttAlert.on('devices', devices => io.emit('SaveDevices', devices));
mqttAlert.on('alert', message => io.emit('Alert', message));

// express addons
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(express.static('public/'));

// express router for rest api
var router = express.Router();

// Job add
router.post('/job', function(req, res){
    mqttAlert.saveJob(req.body.clientId, req.body.job);
    res.json({success : true});
});

// Job delete
router.post('/job/delete', function(req, res){
    mqttAlert.deleteJob(req.body.clientId, req.body.job);
    res.json({success : true});
    io.emit("SaveDevices", mqttAlert.getDevices());
});

// get status logs of devices
router.get('/logs/status', function(req, res){
    res.json({ logs : mqttAlert.state.logs || [] });
});

// api prefix
app.use('/api/v1', router);

// listen on port 8080
http.listen(8080, function () {
  console.log('Example app listening on port 8080!');
});
