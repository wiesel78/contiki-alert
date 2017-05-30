import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'


class Container extends React.Component {

    constructor(props){
        super(props);
    }

    getStatusNameOnce(status){
        if(status == 127) return "Alle";
        if(status | 1)  return "Licht";
        if(status | 2)  return "Temperatur";
        if(status | 4)  return "Uptime";
        if(status | 8)  return "Batterie";
        if(status | 32) return "RSSI";
    }

    render(){
        return (
            <Link to={"/addjob/" + this.props.clientId + "/" + this.props.job.id}>
                <div className="row h4">
                    <div className="col-xs-1">{this.props.job.id}</div>
                    <div className="col-xs-6">{this.props.job.topic}</div>
                    <div className="col-xs-3">{this.getStatusNameOnce(this.props.job.status)}</div>
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
