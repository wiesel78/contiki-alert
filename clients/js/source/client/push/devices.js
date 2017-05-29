import { SaveDevices } from '../redux/'

export const DeviceSubscribes = (socket, store) => {
    socket.on('SaveDevices', function(msg){
        console.log("SaveDevices from socketio : ", msg);
        store.dispatch(SaveDevices(msg));
    });
};
