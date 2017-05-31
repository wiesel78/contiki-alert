import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'

import { AlertItem } from './'

class Container extends React.Component {

    constructor(props){
        super(props);
    }

    render(){

        const items = this.props.alerts.reverse().map((item) =>
             <div key={item.sequence}>
                 <AlertItem item={item} />
                 <hr />
             </div>);

        return (
            <div>
                {items}
            </div>
        );
    };
}

export const AlertListView = withRouter(connect(
    state => ({
        alerts : state.logState.alerts
    })
)(Container));
