import { routerReducer } from 'react-router-redux'
import { DeviceReducers } from './devices'

export const reducers = {
    deviceState : DeviceReducers,
    routing : routerReducer
}
