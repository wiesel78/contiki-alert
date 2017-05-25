import React from 'react';
import { render } from 'react-dom';
import { HashRouter as Router, Route, Link } from 'react-router-dom'

//import DeviceListComponent from './components/DeviceListComponent.jsx';

class About extends React.Component {
  render () {
    return (
      <div>
        About
      </div>
    );
  }
}

class Details extends React.Component {
  render () {
    return (
      <div>
        Details
      </div>
    );
  }
}

class Overview extends React.Component {
  render () {
    return (
      <div>
        Overview
      </div>
    );
  }
}

class App extends React.Component {
  render () {
    return (
      <div className="container">
        <div className="row">
          <div className="col-xs-4">
            <Link to="/">Overview</Link>
          </div>
          <div className="col-xs-4">
            <Link to="/details">Details</Link>
          </div>
          <div className="col-xs-4">
            <Link to="/about">About</Link>
          </div>
        </div>

        <hr/>

        <Route exact path="/" component={Overview}/>
        <Route path="/details" component={Details}/>
        <Route path="/about" component={About}/>
      </div>
    );
  }
}


render(
  <Router>
    <App />
  </Router>, document.getElementById('app'));
