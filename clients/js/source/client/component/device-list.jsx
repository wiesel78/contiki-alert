import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter } from 'react-router-dom'

import { DeviceItem } from './'
import { SaveDevices } from '../redux/devices'

class Container extends React.Component {

    constructor(props){
        super(props);
    }

    render(){
        const items = Object.entries(this.props.devices).map(([key, item]) =>
             <DeviceItem key={key} item={item}></DeviceItem>);

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
