var webpack = require('webpack');
var path = require('path');

var CLIENT_DIR = path.resolve(__dirname, 'src/client')
var BUILD_DIR = path.resolve(__dirname, 'src/client/public');
var APP_DIR = path.resolve(__dirname, 'src/client/app');

var config = {
    entry: APP_DIR + '/index.jsx',
    output:{
        path : BUILD_DIR,
        filename:'bundle.js'
    },
    devServer: {
      inline: true,
      contentBase: CLIENT_DIR,
      port: 8100
    },
    module: {
      rules: [
        {
          test: /.*\.jsx?$/,
          include : APP_DIR,
          exclude: /(node_modules|bower_components)/,
          use: {
            loader: 'babel-loader',
            options: {
              presets: ['es2015', 'react']
            }
          }
        }
      ]
    }
};

module.exports = config;
