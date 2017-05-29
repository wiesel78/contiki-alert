import { immutableSave } from '../utils/immutable'

export const SAVE_DEVICES = "SaveDevices";

export const SaveDevices = (devices = {}) => {
    return { type : SAVE_DEVICES, devices };
};

export const getDevices = (state) => state.deviceState.devices;

const InitialDeviceState = {
  devices : {}
};

export const DeviceReducers = (state = InitialDeviceState, action) =>
{
    switch (action.type)
    {
        case SAVE_DEVICES:
            console.log("SAVE_DEVICES : action : ", action);
            return Object.assign({}, state, { devices : Object.assign({}, state.devices, action.devices) });
        default:
            return state
    }
}
