import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'


class Container extends React.Component {

    constructor(props){
        super(props);
    }

    getStatusNameOnce(status){
        if(status === 127) return "Alle";
        if(status === 1)  return "Licht";
        if(status === 2)  return "Temperatur";
        if(status === 4)  return "Uptime";
        if(status === 8)  return "Batterie";
        if(status === 32) return "RSSI";
    }

    getOperator(operator){
        if(operator === 1) return "kleiner als";
        if(operator === 2) return "größer als";
        return "<";
    }

    render(){
        var items = [];
        const job = this.props.job;

        console.log("job :::: ", job);

        if(job.type == 1){
            items = [
                <div key="id" className="col-xs-5 col-md-1">ID: {job.id}</div>,
                <div key="interval" className="col-xs-7 col-md-2"> Interval: {job.interval}s</div>,
                <div key="status" className="col-xs-5 col-md-2">Wert: {this.getStatusNameOnce(job.status)}</div>,
                <div key="topic" className="col-xs-7 col-md-7">Topic: {job.topic}</div>,
            ];
        }else if(job.type == 2){
            items = [
                <div key="id" className="col-xs-4 col-md-1">ID: {job.id}</div>,
                <div key="duration" className="col-xs-8 col-md-2">Dauer: {job.duration}s</div>,
                <div key="status" className="col-xs-4 col-md-2">{this.getStatusNameOnce(job.status)}</div>,
                <div key="operator" className="col-xs-4 col-md-2">{this.getOperator(job.operator)}</div>,
                <div key="value" className="col-xs-4 col-md-1">{job.value}</div>,
                <div key="topic" className="col-xs-12 col-md-4">Topic: {job.topic}</div>
            ];
        }

        return (
            <Link to={"/addjob/" + this.props.clientId + "/" + this.props.job.id}>
                <div className="row h4 job-item">
                    {items}
                </div>
            </Link>
        );
    };
}

export const JobItem = withRouter(connect(
    state => ({})
)(Container));

JobItem.defaultProps = {
    job : {},
    clientId : ""
};
