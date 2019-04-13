/* 依赖第三方库： MQTT.js
 * 依赖安装：npm install mqtt --save
 * node版本：10.15.3
 * npm版本：6.4.1
 */
var mqtt = require('mqtt')
var events = require('events');

var ADDRESS  = "tcp://192.168.56.4:1883";
var CLIENTID = "Mock-Device";
var USERNAME = "huaqiao";
var PWD      = "1234";
var DATA_TOPIC = "DataTopic";
var PAYLOAD = {"name":"mqtt-device-01","randnum":"520.1314"}
var CMD_TOPIC  = "CommandTopic";
var RESPONSE_TOPIC = "ResponseTopic";

var mock_device = {
  send_actively_interval: null,
  active: "false"
}

var emitter = new events.EventEmitter();

emitter.on('send_actively', function(active) {
  if (active === "true") {
    mock_device.send_actively_interval = setInterval(function(){
      console.log("send data actively from mock device.");
      client.publish(DATA_TOPIC, JSON.stringify(PAYLOAD),function(err){
        if(!err) {
          console.log(err);
        }
      });
    }, 1000);
  } else {
    clearInterval(mock_device.send_actively_interval);
  }
});

var client  = mqtt.connect(ADDRESS,{
  clientId: CLIENTID,
  username: USERNAME,
  password: PWD,
  clean: false,
  protocolId: 'MQIsdp',
  protocolVersion: 3
});

client.on("connect",function(){
  console.log("connect successed.");
  client.subscribe(CMD_TOPIC, function (err) {
    if (!err) {
      console.log("subscribe " + CMD_TOPIC+ " successed.");
    }
  })
});

client.on('error',function(err){
  console.log(err);
});

client.on('message', function (topic, message) {
  console.log(message.toString());
  var msg = JSON.parse(message.toString());
  var cmd = msg['cmd'];
  var method = msg['method'];
  if (cmd === "ping") {
    msg['ping'] = "pong";
  }

  if (cmd === "randnum") {
    msg['randnum'] = "520.1314";
  }

  if (cmd === "message") {
    if (method === "get") {
      msg['message'] = "Are you ok?";
    } else {
      msg['result'] = "set success.";
    }
  }

  if (cmd === "collect") {
    if (method === "get") {
      msg['collect'] = mock_device.active;
    } else {
      mock_device.active = msg['param'];
      emitter.emit('send_actively', msg['param']);
    }
  }
  client.publish(RESPONSE_TOPIC, JSON.stringify(msg));
});
