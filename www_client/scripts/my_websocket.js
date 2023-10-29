function SocketWrapper(init) {
    this.socket = new SimpleWebsocket(init);
    this.messageHandlers = {};

    var that = this;
    this.socket.on('data', function (data) {
        //Extract the message type
        var messageData = JSON.parse(data);
        var messageType = messageData['__MESSAGE__'];
        delete messageData['__MESSAGE__'];

        //If any handlers have been registered for the message type, invoke them
        if (that.messageHandlers[messageType] !== undefined) {
            for (index in that.messageHandlers[messageType]) {
                that.messageHandlers[messageType][index](messageData);
            }
        }
    });
}

SocketWrapper.prototype.on = function (event, handler) {
    //If a standard event was specified, forward the registration to the socket's event emitter
    if (['connect', 'close', 'data', 'error'].indexOf(event) != -1) {
        this.socket.on(event, handler);
    }
    else {
        //The event names a message type
        if (this.messageHandlers[event] === undefined) {
            this.messageHandlers[event] = [];
        }
        this.messageHandlers[event].push(handler);
    }
}

SocketWrapper.prototype.send = function (message, payload) {
    //Copy the values from the payload object, if one was supplied
    var payloadCopy = {};
    if (payload !== undefined && payload !== null) {
        var keys = Object.keys(payload);
        for (index in keys) {
            var key = keys[index];
            payloadCopy[key] = payload[key];
        }
    }

    payloadCopy['__MESSAGE__'] = message;
    this.socket.send(JSON.stringify(payloadCopy));
}

function log(text) {
    document.getElementById('status').innerHTML = text;
}

$(document).ready(function () {
                console.log(window.location.href)
    var socket = new SocketWrapper("ws://"+window.location.host+":8080");

    //Generic events

    socket.on('connect', function () {
        log("socket is connected!");
    });

    socket.on('data', function (data) {
        //log('got message: ' + data)
    });

    socket.on('close', function () {
        log("socket is disconnected!");
    });

    socket.on('error', function (err) {
        log("Error: " + err);
    });

    //Specific message type handlers

    socket.on('machineState', function (args) {
        //log("Received machine state: \"" + args['input'] + "\"");
        //console.log(args);
        document.getElementById('x_pos_id').value = args['x_pos'];
        document.getElementById('y_pos_id').value = args['y_pos'];
        document.getElementById('z_pos_id').value = args['z_pos'];
        document.getElementById('tool_pos_id').value = args['tool_pos'];
    });

                document.getElementById('cam_id').src = "http://"+window.location.host+":8888";
});