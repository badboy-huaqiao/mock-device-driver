#!/usr/bin/python
# -*- coding: UTF-8 -*-

#用于模拟一个基于mqtt协议传输虚拟设备驱动

import paho.mqtt.client as mqtt
import json

#当接收到命令，响应命令
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload)+'\n')
    d = json.loads(msg.payload)

    if d['cmd'] == "message":
       if d['method'] == "get":
          d['message'] = "Are you ok?"
       elif d['method'] == "set":
          d['result'] = "set successed."
    if d['cmd'] == "ping":
       print("This is ping cmd")
       d['ping'] = "pong"

    print(json.dumps(d))
    client.publish("ResponseTopic", json.dumps(d))

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    #监听命令
    client.subscribe("CommandTopic")

client = mqtt.Client()
client.username_pw_set("huaqiao", "1234")
client.on_message = on_message
client.on_connect = on_connect

client.connect("192.168.56.4", 1883, 60)

client.loop_forever()
