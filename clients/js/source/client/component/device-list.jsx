import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'

import { DeviceItem } from './'
import { SaveDevices } from '../redux/devices'

class Container extends React.Component {

    constructor(props){
        super(props);
    }

    render(){
        const items = Object.entries(this.props.devices).map(([key, item]) =>
             <Link to={"/details/" + key} key={key}>
                 <DeviceItem item={item} />
             </Link>);

        return (
            <div>
                {items}
            </div>
        );
    };
}

export const DeviceListView = withRouter(connect(
    state => ({
        devices : state.deviceState.devices
    }),
    { SaveDevices }
)(Container));
