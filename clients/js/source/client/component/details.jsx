import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'

import { SaveDevices } from '../redux/'

class Container extends React.Component {

    constructor(props){
        super(props);
    }

    render(){

        const devices = [];

        if(this.props.match && this.props.match.params.clientId &&
            this.props.devices[this.props.match.params.clientId])
        {
            console.log("match ", this.props.match.params.clientId, " devices ", this.props.devices);
            const device = this.props.devices[this.props.match.params.clientId];

            devices.push(device);
        }else{
            Object.entries(this.props.devices).map(([key,item]) => devices.push(item));
        }

        const details = devices.map(item =>
            <div key={item.clientId}>
                <h2>{item.clientId}</h2>

                <Link to={"/addjob/" + item.clientId} className="btn btn-primary">Job erstellen</Link>
            </div>);

        return (
            <div>
                details
                <hr />
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
