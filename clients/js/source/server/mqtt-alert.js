var mqtt = require('mqtt');

const rawDevice = { clientId : -1, jobs : {}, status : {}, deletedJobs : []};

class MqttAlert {

    constructor(){

        this.state = {
            devices : {},
            logs : [],
            subscribes : [
                {
                    topic : '#',
                    type : 1
                }
            ]
        }

        this.listener = {
            alert : [],
            status : [],
            client : [],
            job : [],
            device : [],
            devices : []
        }

        console.log("hallo welt mqtt alert module");
    }

    on(listen, action){
        if(typeof action !== 'function')
            return;

        if(this.listener[listen] instanceof Array)
            this.listener[listen].push(action);
    }

    callListener(listen, value){
        this.listener[listen].map(action => action(value));

        if(listen === 'device')
            this.callListener('devices', this.state.devices);
    }

    saveJob(clientId, job){
        console.log("/job router : clientId : ", clientId, " job : ", job);

        var topic = "job/add/{type}{clientId}";
        if(job.type == 1) topic = topic.replace("{type}", "status");
        if(job.type == 2) topic = topic.replace("{type}", "alert");
        topic = topic.replace("{clientId}", clientId ? "/" + clientId : "");

        this.client.publish(topic, JSON.stringify(job));
    }

    deleteJob(clientId, job){
        console.log("/job/delete router : clientId : ", clientId, " job : ", job);

        var topic = "job/delete/{type}{clientId}";
        if(job.type == 1) topic = topic.replace("{type}", "status");
        if(job.type == 2) topic = topic.replace("{type}", "alert");
        topic = topic.replace("{clientId}", clientId ? "/" + clientId : "");

        this.client.publish(topic, JSON.stringify({jobId:job.id}));

        console.log("löschen von clientId : ", clientId, " jobid : ", job.id);
        this.deleteJobById(clientId, job.id);
    }

    deleteJobById(clientId, id) {
        const device = this.state.devices[clientId];
        if(!device)
            return;

        const job = device.jobs[id];
        if(!job)
            return;

        if(device.deletedJobs.every(x => x != id))
            device.deletedJobs.push(id);

        device.jobs[id] = undefined;
    };

    getDevices(){
        return this.state.devices;
    }

    connect(listen){
        this.client = mqtt.connect(listen);

        this.client.on('connect', () => {
            this.state.subscribes.map(sub => this.client.subscribe(sub.topic));
        });

        this.client.on('message', (topic, message) => {
            var msg = {};

            try{
                var msg = JSON.parse(message.toString());
            }catch(e){return;}

            console.log("mqtt message receive :", topic);
            console.log("message clientId : ", msg);
            /* wenn sich ein sensorknoten als aktiv meldet */
            if(topic.match(/^clients\/.*/))
            {
                this.state.devices[msg.clientId] = Object.assign({}, rawDevice, { clientId : msg.clientId });
                this.state.devices[msg.clientId].status = Object.assign({}, msg);
                this.state.devices[msg.clientId].date = Date.now();

                this.callListener('client', this.state.devices[msg.clientId]);
                this.callListener('device', this.state.devices[msg.clientId]);
            }

            /* wenn ein job hinzugefuegt wurde job/details/clienId/jobId */
            if(topic.match(/^job\/details\/(.*)\/(\d)/)){
                this.state.devices[msg.clientId] = Object.assign({}, rawDevice, this.state.devices[msg.clientId] || {});
                this.state.devices[msg.clientId].jobs[msg.job.id] = Object.assign({}, msg.job);
                this.state.devices[msg.clientId].date = Date.now();

                    console.log("füge job hinzu ", msg, this.state.devices[msg.clientId].jobs[msg.job.id]);
                this.callListener('job', this.state.devices[msg.clientId].jobs[msg.job.id]);
                this.callListener('device', this.state.devices[msg.clientId]);
            }

            /* Wenn ein alarm gemeldet wird */
            if(topic.match(/^alert/)){
                this.state.devices[msg.clientId].date = Date.now();

                this.callListener('alert', msg);
                this.callListener('device', this.state.devices[msg.clientId]);
            }

            /* Wenn ein status gemeldet wird */
            if(topic.match(/^status/)){

                this.state.devices[msg.clientId] = Object.assign({}, rawDevice, this.state.devices[msg.clientId] || {});
                this.state.devices[msg.clientId].status = Object.assign({}, this.state.devices[msg.clientId].status || {}, msg);
                this.state.devices[msg.clientId].date = Date.now();

                this.logStatus(msg.clientId, msg);

                this.callListener('status', msg);
                this.callListener('device', this.state.devices[msg.clientId]);
            }

        });
    }

    logStatus(clientId, message) {
        var logEntry = {date : Date.now(), clientId, type : 1, message};
        this.state.logs.push(logEntry);
        return logEntry;
    }

}

module.exports = new MqttAlert();
