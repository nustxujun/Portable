var net = require('net');

var curConn;
var receiveCallback;

class Connection
{
    constructor(socket){
        this.socket = socket;
        socket.on("data", function (data)
        {
            if (receiveCallback)
                receiveCallback(JSON.parse(data));
            console.log(data);
        }).on("close", function (){
            console.log("disconnect");

            curConn = null;
        }).on("error", function (err){
            console.log(err);
            curConn = null;
        })
    }

    send(msg){
        var size = new Buffer(4);
        size.writeUInt32LE(msg.length,0);

        this.socket.write(size.toString("binary"));
        this.socket.write(msg);
    }
}

exports.init = function (port){
    net.createServer(function (sock){
        console.log('CONNECTED' + sock.remoteAddress + ':' + sock.remotePort);

        curConn = new Connection(sock);
    }).listen(port, "127.0.0.1");
    console.log("listening on port: " + port);
}


exports.send = function (msg){
    curConn.send(msg);
}

exports.receive = function (callback){
    receiveCallback= callback;
}