import React from 'react'

export class AlertItem extends React.Component {

    constructor(props){
        super(props);
    }

    render(){
        console.log("ssss : ", this.props.item);

        const alert = this.props.item;

        return (
            <div>
                <div className="text-center visible-xs-block">
                    {new Date(alert.date).toLocaleTimeString()}, job:{alert.jobId}, Gerät:{alert.clientId}
                </div>
                <h3 className="text-center hidden-xs">
                    {new Date(alert.date).toLocaleTimeString()}, job:{alert.jobId}, Gerät:{alert.clientId}
                </h3>
            </div>
        );
    };
}

AlertItem.defaultProps = {
    item : {}
};
