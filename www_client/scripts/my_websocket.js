let cam_img_raw = 0;
var socket_ws = 0;

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

function logStatus(text) {
    if (document.getElementById('status'))
        document.getElementById('status').innerHTML = text;
}

$(document).ready(function () {
    //console.log(window.location.href)
    socket_ws = new SocketWrapper("ws://" + window.location.host + ":8080");

    //Generic events

    socket_ws.on('connect', function () {
        logStatus("socket is connected!");
    });

    socket_ws.on('data', function (data) {
        //log('got message: ' + data)
    });

    socket_ws.on('close', function () {
        logStatus("socket is disconnected!");
    });

    socket_ws.on('error', function (err) {
        logStatus("Error: " + err);
    });

    //Specific message type handlers

    socket_ws.on('machineState', function (args) {
        //log("Received machine state: \"" + args['input'] + "\"");
        //console.log("x:" + args['x_pos'] + ", y:" + args['y_pos'] + ", z:" + args['z_pos'] + ", tool:" + args['tool_pos']);

        document.getElementById('x_pos_id').value = parseFloat(args['x_pos']).toFixed(3);
        document.getElementById('y_pos_id').value = parseFloat(args['y_pos']).toFixed(3);
        document.getElementById('z_pos_id').value = parseFloat(args['z_pos']).toFixed(3);
        document.getElementById('tool_pos_id').value = parseFloat(args['tool_pos']).toFixed(1);
        document.getElementById('x_ref_id').value = parseFloat(args['x_ref']).toFixed(3);
        document.getElementById('y_ref_id').value = parseFloat(args['y_ref']).toFixed(3);
        document.getElementById('z_ref_id').value = parseFloat(args['z_ref']).toFixed(3);
    });

/*
    window.addEventListener('keydown', function (e) {
        let x_pos = this.document.getElementById('x_pos_id').value;
        let y_pos = this.document.getElementById('y_pos_id').value;
        let new_x_pos, new_y_pos;

        console.log("Curr pos x:" + x_pos + " y:" + y_pos);
        //console.log("Key pressed:"+e.key);
        if (e.key == 'ArrowUp') {
            // up arrow
            console.log("key up");
            new_x_pos = x_pos;
            new_y_pos = y_pos + 1;
            setMachineXYPos(new_x_pos, new_y_pos);
        }
        else if (e.key == 'ArrowDown') {
            // down arrow
            console.log("key down");
            //setMachineXYPos(x_pos, y_pos + 1.0);
        }
        else if (e.key == 'ArrowLeft') {
            // left arrow
            console.log("key left");
            //setMachineXYPos(x_pos - 1.0, y_pos);
        }
        else if (e.key == 'ArrowRight') {
            // right arrow
            console.log("key right");
            //setMachineXYPos(x_pos + 1.0, y_pos);
        }
    })
    */
});

