const path = require('path');
var CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = {
    // watch : true,
    context : __dirname,
    entry : './source/client/index.jsx',
    output : {
        path : path.resolve('public'),
        filename : 'build.js'
    },
    devtool : "source-map",
    module : {
        loaders : [
            {
                test : /\.jsx?$/,
                loader : 'babel-loader',
                query : {
                    "presets" : ['es2015', 'react']
                }
            }
        ]
    },
    plugins: [
        new CopyWebpackPlugin([
            { from: 'source/client/index.html', to: 'index.html' },
            { from: 'source/client/main.css', to: 'main.css' },
            { from: 'node_modules/socket.io-client/dist/socket.io.js', to : 'socket.io.js' },
            { from: 'node_modules/bootstrap/dist/css/bootstrap.min.css', to : 'bootstrap.min.css' },
            { from: 'node_modules/bootstrap/dist/css/bootstrap.min.css.map', to : 'bootstrap.min.css.map' },
            { from: 'node_modules/font-awesome/css/font-awesome.min.css', to : 'font-awesome.min.css' },
            { from: 'node_modules/font-awesome/css/font-awesome.css.map', to : 'font-awesome.css.map' },
            { from: 'node_modules/font-awesome/fonts', to : 'fonts' }
        ], {
            // By default, we only copy modified files during
            // a watch or webpack-dev-server build. Setting this
            // to `true` copies all files.
            copyUnmodified: true
        })
    ]
};
