import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'

import { saveJobsRest } from '../rest/'
import { SaveDevices } from '../redux/'

class Container extends React.Component {

    constructor(props){
        super(props);

        this.state = {
            id : -1,
            type : 2,
            topic : "alert/light",
            status : 1,
            value : 14000,
            interval : 30,
            duration : 30,
            operator : 2
        };


        this.handleClick = this.handleClick.bind(this);
        this.handleTopicChange = this.handleTopicChange.bind(this);
        this.handleStatusChange = this.handleStatusChange.bind(this);
        this.handleValueChange = this.handleValueChange.bind(this);
        this.handleDurationChange = this.handleDurationChange.bind(this);
        this.handleOperatorChange = this.handleOperatorChange.bind(this);
        this.handleTypeChange = this.handleTypeChange.bind(this);
        this.handleIntervalChange = this.handleIntervalChange.bind(this);
    }

    handleClick(event){
        if(this.props.match && this.props.match.params.clientId){
            const clientId = this.props.match.params.clientId;
            saveJobsRest(clientId, this.state);
        }else{
            saveJobsRest("", this.state);
        }
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
        this.setState({ type : parseInt(event.target.value) });
    }

    handleIntervalChange(event){
        this.setState({ interval : parseInt(event.target.value) });
    }

    render(){
        console.log(this.state);

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
                <button className="btn btn-primary btn-lg btn-block" onClick={this.handleClick}>Job erstellen</button>
            </form>
        );
    };
}

export const AddJobView = withRouter(connect(
    state => ({
        devices : state.deviceState.devices
    }),
    { SaveDevices }
)(Container));
