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
                <button onClick={this.handleClick}>click me</button>
                Home component {this.props.item.id}
            </div>
        );
    };
}

DeviceItem.defaultProps = {
    item : null,
};
