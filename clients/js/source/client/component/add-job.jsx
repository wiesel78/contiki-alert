import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'
import { push } from 'react-router-redux';

import { saveJobsRest, deleteJobsRest } from '../rest/'
import { SaveDevices } from '../redux/'

class Container extends React.Component {

    constructor(props){
        super(props);

        this.state = {
            id : -1,
            type : 2,
            topic : "alert",
            status : 1,
            interval : 30,
            duration : 30,
            value : 14000,
            operator : 2
        };

        var device = props.devices[props.match.params.clientId];
        if(device){
            var job = device.jobs[props.match.params.jobId];
            if(job){
                this.state = {
                    id : job.id,
                    type : job.type,
                    topic : job.topic,
                    status : job.status,
                    interval : job.interval ? job.interval : 30,
                    duration : job.duration ? job.duration : 30,
                    value : job.value ? job.value : 14000,
                    operator : job.operator ? job.operator : 2
                };
            }
        }




        this.handleDelete = this.handleDelete.bind(this);
        this.handleClick = this.handleClick.bind(this);
        this.handleTopicChange = this.handleTopicChange.bind(this);
        this.handleStatusChange = this.handleStatusChange.bind(this);
        this.handleValueChange = this.handleValueChange.bind(this);
        this.handleDurationChange = this.handleDurationChange.bind(this);
        this.handleOperatorChange = this.handleOperatorChange.bind(this);
        this.handleTypeChange = this.handleTypeChange.bind(this);
        this.handleIntervalChange = this.handleIntervalChange.bind(this);
    }

    handleDelete(){
        const device = this.props.devices[this.props.match.params.clientId];
        if(!device)
            return;

        const job = device.jobs[this.props.match.params.jobId];
        if(!job)
            return;

        deleteJobsRest(this.props.match.params.clientId, job)
        .then(() => {
            console.log("lösch request erfolgreich uebermittelt");
            this.props.history.push('/');
        });
    }

    handleClick(event){
        let defer;

        if(this.props.match && this.props.match.params.clientId){
            const clientId = this.props.match.params.clientId;
            defer = saveJobsRest(clientId, this.state);
        }else{
            defer = saveJobsRest("", this.state);
        }

        defer.then(() => {
            console.log("erfolgreich gesendet");
            this.props.history.push('/');
        });
    }

    handleTopicChange(event){
        this.setState({ topic : event.target.value });
    }

    handleStatusChange(event){
        this.setState({ status : parseInt(event.target.value) });
    }

    handleValueChange(event){
        this.setState({ value : parseInt(event.target.value) });
    }

    handleDurationChange(event){
        this.setState({ duration : parseInt(event.target.value) });
    }

    handleOperatorChange(event){
        this.setState({ operator : parseInt(event.target.value) });
    }

    handleTypeChange(event){
        let type = parseInt(event.target.value);
        if(this.state.topic == "alert" && type == 1){
            this.setState({ topic : "status" });
        }else if(this.state.topic == "status" && type == 2){
            this.setState({ topic : "alert" });
        }

        this.setState({ type });
    }

    handleIntervalChange(event){
        this.setState({ interval : parseInt(event.target.value) });
    }

    render(){
        console.log(this.props);

        return (
            <form>
                <div className="form-group">
                    <label htmlFor="type"></label>
                    <select className="form-control input-lg" id="type" value={this.state.type} onChange={this.handleTypeChange}>
                        <option value="1">Status</option>
                        <option value="2">Alarm</option>
                    </select>
                </div>
                <div className="form-group">
                    <label htmlFor="topic">Topic</label>
                    <input className="form-control input-lg" id="topic" value={this.state.topic} onChange={this.handleTopicChange}/>
                </div>
                <div className="form-group">
                    <label htmlFor="status">Welcher Status soll überwacht werden</label>
                    <select className="form-control input-lg" id="status" value={this.state.status} onChange={this.handleStatusChange}>
                        <option value="1">Licht</option>
                        <option value="2">Temperatur</option>
                        <option value="4">Uptime</option>
                        <option value="8">Battery</option>
                        <option value="32">Signalstärke</option>

                        <option value="127" className={(this.state.type != 1 ? "hidden" : "")}>Alle</option>
                    </select>
                </div>

                <div className={"form-group " + (this.state.type != 1 ? "hidden" : "")}>
                    <label htmlFor="interval">Interval</label>
                    <input className="form-control input-lg" id="interval" type="number" value={this.state.interval} onChange={this.handleIntervalChange}/>
                </div>

                <div className={"form-group " + (this.state.type != 2 ? "hidden" : "")}>
                    <label htmlFor="borderValue">Grenzwert</label>
                    <input className="form-control input-lg" id="borderValue" type="number" value={this.state.value} onChange={this.handleValueChange}/>
                </div>
                <div className={"form-group " + (this.state.type != 2 ? "hidden" : "")}>
                    <label htmlFor="duration">Alarm auslösen wenn Grenzwert für</label>

                    <div className="input-group">
                        <input className="form-control input-lg" id="duration" type="number" value={this.state.duration} onChange={this.handleDurationChange}/>
                        <div className="input-group-addon">Sekunden</div>
                    </div>

                    <select className="form-control input-lg" id="operator" value={this.state.operator} onChange={this.handleOperatorChange}>
                        <option value="2">überschritten wird</option>
                        <option value="1">unterschritten wird</option>
                    </select>
                </div>
                <button className="btn btn-primary btn-lg btn-block" onClick={this.handleClick}>
                    Job speichern
                </button>
                <button className={"btn btn-danger btn-lg btn-block " + (this.props.match.params.jobId ? "" : "hidden")} onClick={this.handleDelete}>Job löschen</button>
            </form>
        );
    };
}

export const AddJobView = withRouter(connect(
    state => ({
        devices : state.deviceState.devices
    }),
    { SaveDevices, push }
)(Container));
