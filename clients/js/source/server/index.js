
var mqtt = require('mqtt');
var express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var bodyParser = require('body-parser');

const rawDevice = { clientId : -1, jobs : {}, status : {}, deletedJobs : []};
var state = {
    devices : {},
    logs : [],
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

const logStatus = (clientId, message) => {
    state.logs.push({date : Date.now(), clientId, type : 1, message});
}

// löscht einen job aus einem geräteeintrag und fügt die jobid der gelöscht liste hinzu
const deleteJob = (clientId, id) => {
    const device = state.devices[clientId];
    if(!device)
        return;

    const job = device.jobs[id];
    if(!job)
        return;

    if(device.deletedJobs.every(x => x != id))
        device.deletedJobs.push(id);

    device.jobs[id] = undefined;
};

// SOCKET-IO
io.on('connection', function(socket){
  console.log('socket.io user connected');
  io.emit("SaveDevices", state.devices);
});



// MQTT
var client = mqtt.connect('mqtt://localhost:1883');

client.on('connect', () => {
    state.subscribes.map(sub => client.subscribe(sub.topic));
});

client.on('message', (topic, message) => {
    var msg = JSON.parse(message.toString());

    console.log("mqtt message receive :", topic);
    console.log("message clientId : ", msg.clientId);
    /* wenn sich ein sensorknoten als aktiv meldet */
    if(topic.match(/^clients\/.*/))
    {
        state.devices[msg.clientId] = Object.assign({}, rawDevice, { clientId : msg.clientId });
        state.devices[msg.clientId].jobs = {};
        state.devices[msg.clientId].status = Object.assign({}, msg);
        state.devices[msg.clientId].date = Date.now();

        io.emit("SaveDevices", state.devices);
    }

    /* wenn ein job hinzugefuegt wurde job/details/clienId/jobId */
    if(topic.match(/^job\/details\/(.*)\/(\d)/)){
        state.devices[msg.clientId] = Object.assign({}, rawDevice, state.devices[msg.clientId] || {});
        state.devices[msg.clientId].jobs[msg.job.id] = Object.assign({}, msg.job);
        state.devices[msg.clientId].date = Date.now();

        io.emit("SaveDevices", state.devices);
    }

    /* Wenn ein alarm gemeldet wird */
    if(topic.match(/^alert/)){
        state.devices[msg.clientId].date = Date.now();

        io.emit("Alert", msg);
    }

    /* Wenn ein status gemeldet wird */
    if(topic.match(/^status/)){

        state.devices[msg.clientId] = Object.assign({}, rawDevice, state.devices[msg.clientId] || {});
        state.devices[msg.clientId].status = Object.assign({}, state.devices[msg.clientId].status || {}, msg);
        state.devices[msg.clientId].date = Date.now();

        logStatus(msg.clientId, msg);

        io.emit("SaveDevices", state.devices);
    }

});



app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(express.static('public/'));

var router = express.Router();
router.post('/job', function(req, res){
    console.log("/job router : ", req.body);

    var topic = "job/add/{type}{clientId}";
    if(req.body.job.type == 1) topic = topic.replace("{type}", "status");
    if(req.body.job.type == 2) topic = topic.replace("{type}", "alert");
    topic = topic.replace("{clientId}", req.body.clientId ? "/" + req.body.clientId : "");

    client.publish(topic, JSON.stringify(req.body.job));

    res.json({success : true});
});
router.post('/job/delete', function(req, res){
    console.log("/job/delete router : ", req.body);

    var topic = "job/delete/{type}{clientId}";
    if(req.body.job.type == 1) topic = topic.replace("{type}", "status");
    if(req.body.job.type == 2) topic = topic.replace("{type}", "alert");
    topic = topic.replace("{clientId}", req.body.clientId ? "/" + req.body.clientId : "");

    client.publish(topic, JSON.stringify({jobId:req.body.job.id}));

    console.log("löschen von clientId : ", req.body.clientId, " jobid : ", req.body.job.id);
    deleteJob(req.body.clientId, req.body.job.id);

    res.json({success : true});

    io.emit("SaveDevices", state.devices);
});
router.get('/logs/status', function(req, res){
    console.log("logs/status");

    res.json({ logs : state.logs || [] });
});
app.use('/api/v1', router);

http.listen(8080, function () {
  console.log('Example app listening on port 8080!');
});
