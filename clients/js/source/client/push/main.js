import { DeviceSubscribes } from './'

export const socket = io();
export const CreateSubscribes = (socket, store) => {
    DeviceSubscribes(socket, store);
}
