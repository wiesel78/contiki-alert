import React from 'react'
import { bindActionCreators } from 'redux'
import { connect } from 'react-redux'
import { withRouter, Link } from 'react-router-dom'

import { JobItem } from './'


class Container extends React.Component {

    constructor(props){
        super(props);
    }

    getJobsByType(){
        return Object.entries(this.props.device.jobs)
            .filter(([key, job]) => job.type == this.props.type)
            .map(([key, job]) =>
                <JobItem key={job.id} clientId={this.props.device.clientId} job={job} />
            );
    }

    render(){
        return (
            <div>
                {this.getJobsByType()}
            </div>
        );
    };
}

export const JobItemList = withRouter(connect(
    state => ({})
)(Container));

JobItemList.defaultProps = {
    device : {},
    type : 1
};
