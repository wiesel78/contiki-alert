import { SaveDevices, LogAlertMessage } from '../redux/'

export const DeviceSubscribes = (socket, store) => {
    socket.on('SaveDevices', function(msg){
        console.log("devices ", msg);
        store.dispatch(SaveDevices(msg));
    });

    socket.on('Alert', function(msg){
        console.log("alarm ertönt", msg);
        store.dispatch(LogAlertMessage(msg));
        window.navigator.vibrate(1000);
    });
};
