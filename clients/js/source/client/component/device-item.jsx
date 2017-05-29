import React from 'react'

export class DeviceItem extends React.Component {

    constructor(props){
        super(props);

        this.handleClick = this.handleClick.bind(this);
    }

    handleClick(event){
        console.log("asdasdasd");
    }

    render(){
        console.log("ssss : ", this.props.item);

        return (
            <div>
                <h3>{this.props.item.clientId}</h3>
            </div>
        );
    };
}

DeviceItem.defaultProps = {
    item : null,
};
