
var mqtt = require('mqtt');
var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var bodyParser = require('body-parser');

const rawDevice = { clientId : -1, jobs : {}, status : {}};
var state = {
    devices : {},
    subscribes : [
        {
            topic : 'clients/+',
            log : [],
            type : 3
        },
        {
            topic : 'job/details/#',
            log : [],
            type : 4
        },
        {
            topic : 'status/#',
            log : [],
            type : 1
        },
        {
            topic : 'alert/#',
            log : [],
            type : 2
        }
    ]
}



// SOCKET-IO
io.on('connection', function(socket){
  console.log('socket.io user connected', state.devices);
  io.emit("SaveDevices", state.devices);
});



// MQTT
var client = mqtt.connect('mqtt://localhost:1883');

client.on('connect', () => {

    state.subscribes.map(sub => client.subscribe(sub.topic));
    // client.subscribe('clients/+');
    // client.subscribe('job/details/#');
});

client.on('message', (topic, message) => {
    var msg = JSON.parse(message.toString());

    console.log(topic, " : ", msg);

    /* wenn sich ein sensorknoten als aktiv meldet */
    if(topic.match(/^clients\/.*/))
    {
        state.devices[msg.clientId] = Object.assign({}, rawDevice, msg);

        io.emit("SaveDevices", state.devices);
    }

    /* wenn ein job hinzugefuegt wurde job/details/clienId/jobId */
    if(topic.match(/^job\/details\/(.*)\/(\d)/)){
        state.devices[msg.clientId] = Object.assign({}, rawDevice, state.devices[msg.clientId] || {});
        state.devices[msg.clientId].jobs[msg.job.id] = Object.assign({}, msg.job);

        console.log("job hinzugefuegt");

        io.emit("SaveDevices", state.devices);
    }

    if(topic.match(/^alert/)){
        console.log("alarm ");

        io.emit("Alert", msg);
    }

    if(topic.match(/^status/)){
        console.log("status ");

        

        io.emit("SaveDevices", state.devices);
    }

});



app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(express.static('public/'));

var router = express.Router();
router.post('/job', function(req, res){
    console.log(req.body);

    var topic = "job/add/{type}{clientId}";
    if(req.body.job.type == 1) topic = topic.replace("{type}", "status");
    if(req.body.job.type == 2) topic = topic.replace("{type}", "alert");
    topic = topic.replace("{clientId}", req.body.clientId ? "/" + req.body.clientId : "");

    client.publish(topic, JSON.stringify(req.body.job));
})
app.use('/api/v1', router);

http.listen(8080, function () {
  console.log('Example app listening on port 8080!');
});
