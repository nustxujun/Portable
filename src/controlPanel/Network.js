var net = require('net');

var curConn;
var receiveCallback;

class Connection
{
    constructor(socket){
        this.msgsize = 4;
        this.getBody = false;
        this.mCache = Buffer.alloc(2048);
        this.mCacheOffset = 0;
        this.socket = socket;
        curConn = this;
        socket.on("data", function (data)
        {
            var head = 0;
            var tail = data.length;

            while(head < tail)
            {
                var need = curConn.msgsize - curConn.mCacheOffset ;
                var read = Math.min(tail - head, need);

                for (var i = 0; i < need; ++i)
                {
                    curConn.mCache.writeUInt8(data.readUInt8(head),curConn.mCacheOffset );
                    head += 1;
                    curConn.mCacheOffset += 1;
                }

                var count = curConn.mCacheOffset;
                if (count == curConn.msgsize)
                {
                    if (curConn.getBody)
                    {
                        var msg = curConn.mCache.toString('utf8',0,curConn.mCacheOffset );
                        console.log();
                        if (receiveCallback)
                            receiveCallback(JSON.parse(msg));
                            curConn.getBody = false;
                            curConn.msgsize = 4;
                    }
                    else
                    {
                        curConn.msgsize = curConn.mCache.readUInt8(0);
                        curConn.getBody = true;
                    }
                    curConn.mCacheOffset = 0;

                }
                else
                    console.error("wtf!");
                    
            }

        }).on("close", function (){
            console.log("disconnect");

            curConn = null;
        }).on("error", function (err){
            console.log(err);
            curConn = null;
        })
    }

    send(msg){
        var str = JSON.stringify(msg);
        var size = Buffer.alloc(4);
        size.writeUInt32LE(str.length,0);

        this.socket.write(size.toString("binary"));
        this.socket.write(str);
    }
}

exports.init = function (port){
    net.createServer(function (sock){
        console.log('CONNECTED' + sock.remoteAddress + ':' + sock.remotePort);

        new Connection(sock);
    }).listen(port, "127.0.0.1");
    console.log("listening on port: " + port);
}


exports.send = function (msg){
    curConn.send(msg);
}

exports.receive = function (callback){
    receiveCallback= callback;
}