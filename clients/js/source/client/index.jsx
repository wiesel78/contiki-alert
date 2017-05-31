import React from 'react'
import ReactDom from 'react-dom'
import { HashRouter as Router, Route, Link } from 'react-router-dom'
import { Provider } from 'react-redux'

import createHistory from 'history/createBrowserHistory'
import { createStore, combineReducers } from 'redux'
import { socket, CreateSubscribes } from './push/'
import { DeviceListView, DetailsView, AddJobView, AlertListView, AlertCounter } from './component/'
import { reducers, SaveDevices, DeviceReducers } from './redux/'

// Create a history of your choosing (we're using a browser history in this case)
let history = createHistory();
let store = createStore(combineReducers(reducers));

//store.subscribe(() => console.log(store.getState()));

CreateSubscribes(socket, store);

ReactDom.render(
    <Provider store={store}>
        <Router history={history}>
            <div className="container">
                <div className="navigation">
                    <Link className="navigation-item" to="/">Geräte</Link>
                    <Link className="navigation-item" to="/details">Details</Link>
                    <Link className="navigation-item" to="/addjob">Job</Link>
                    <Link className="navigation-item" to="/alerts">Alarm <AlertCounter /></Link>
                </div>

                <Route exact path="/" component={DeviceListView}/>

                <Route exact path="/details/:clientId" component={DetailsView}/>
                <Route exact path="/details" component={DetailsView}/>

                <Route exact path="/addjob/:clientId/:jobId" component={AddJobView}/>
                <Route exact path="/addjob/:clientId" component={AddJobView}/>
                <Route exact path="/addjob" component={AddJobView}/>

                <Route exact path="/alerts" component={AlertListView}/>
            </div>
        </Router>
    </Provider>,
    document.getElementById('app'));
