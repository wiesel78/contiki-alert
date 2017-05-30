import { routerReducer } from 'react-router-redux'
import { DeviceReducers, LogReducers } from './'

export const reducers = {
    deviceState : DeviceReducers,
    logState : LogReducers,
    routing : routerReducer
}
