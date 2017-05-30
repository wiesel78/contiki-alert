import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'

import { SaveDevices } from '../redux/'
import { JobItem, JobItemList, DeviceItem } from './'

class Container extends React.Component {

    constructor(props){
        super(props);
    }

    render(){

        const devices = [];
        const device = this.props.devices[this.props.match.params.clientId];
        if(device){
            devices.push(device);
        }else{
            Object.entries(this.props.devices).map(([key,item]) => devices.push(item));
        }

        const details = devices.map(item =>
            <div key={item.clientId}>
                <DeviceItem item={item} extended="true"/>
                <h4>Status Jobs</h4>
                    <JobItemList device={item} type="1"/>
                <h4>Alarm Jobs</h4>
                    <JobItemList device={item} type="2"/>
                <Link to={"/addjob/" + item.clientId} className="btn btn-primary">Job erstellen</Link>
                <hr />
            </div>);

        return (
            <div>
                {details}
            </div>
        );
    };
}

export const DetailsView = withRouter(connect(
    state => ({
        devices : state.deviceState.devices
    }),
    { SaveDevices }
)(Container));
