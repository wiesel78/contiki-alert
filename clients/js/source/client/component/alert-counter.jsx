import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'

class Container extends React.Component {

    constructor(props){
        super(props);
    }

    render(){

        return (
            <span className="alert-counter text-warning">{this.props.alerts.length}</span>
        );
    };
}

export const AlertCounter = connect(
    state => ({
        alerts : state.logState.alerts
    })
)(Container);
