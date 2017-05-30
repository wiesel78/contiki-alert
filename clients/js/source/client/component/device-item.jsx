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

        const device = this.props.item;
        const stats = [
            <span key="ipv6" className="detail-span">ipv6 : {device.status.ipv6} </span>,
            <span key="last" className="detail-span">letzte Nachricht : {device.date ? new Date(device.date).toLocaleTimeString() : "/"}</span>
        ];

        if(this.props.extended)
        {
            if(device.status.sequence)
                stats.push(<span key="sequence" className="detail-span">gesenete Nachrichten : {device.status.sequence} </span>);

            if(device.status.light)
                stats.push(<span key="light" className="detail-span">Licht : {device.status.light} mL </span>);

            if(device.status.temperature)
                stats.push(<span key="temperature" className="detail-span">Temperatur : {device.status.temperature} mC° </span>);

            if(device.status.uptime)
                stats.push(<span key="uptime" className="detail-span">Uptime : {device.status.uptime} s </span>);

            if(device.status.power)
                stats.push(<span key="power" className="detail-span">Batterie : {device.status.power} mV </span>);

            if(device.status.signal)
                stats.push(<span key="signal" className="detail-span">Signal : {device.status.signal} dBm </span>);
        }


        return (
            <div>
                <h3>{this.props.item.clientId}</h3>
                <div className="device-status-list">
                    {stats}
                </div>
            </div>
        );
    };
}

DeviceItem.defaultProps = {
    item : null,
    extended : false
};
