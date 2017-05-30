
export const SAVE_ALERT_LOGS = "SaveAlertLogs";
export const SAVE_STATUS_LOGS = "SaveStatusLogs";

export const SaveAlertLogs = (alerts = []) => {
    return { type : SAVE_ALERT_LOGS, alerts };
};

export const SaveStatusLogs = (stats = []) => {
    return { type : SAVE_STATUS_LOGS, stats };
};

// export const getDevices = (state) => state.deviceState.devices;

const InitialLogState = {
    alerts : [],
    stats : []
};

export const LogReducers = (state = InitialLogState, action) =>
{
    switch (action.type)
    {
        case SAVE_ALERT_LOGS:
            return Object.assign({}, state,
                { alerts : [ ...state.alerts, action.alerts] });
        case SAVE_STATUS_LOGS:
            console.log("state status : ", state);
            return Object.assign({}, state,
                { stats : [ ...state.stats, action.stats] });
        default:
            return state;
    }
}
