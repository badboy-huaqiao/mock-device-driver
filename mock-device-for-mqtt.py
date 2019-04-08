#!/usr/bin/python
# -*- coding: UTF-8 -*-

#用于模拟一个基于mqtt协议传输虚拟设备驱动

import paho.mqtt.client as mqtt
import json
import time
import Queue
import threading

globalQueue = Queue.Queue()

def send_data():
    #java版本, name的值为添加的设备名
    var data = {"randnum":520.1314,"name":""}

    #go版本, name的值为添加的设备名, go版本的区别是必须带上cmd字段
    #var data = {"randnum":520.1314,"name":"","cmd":"randnum"}

    print("sending data actively! " + json.dumps(data))
    client.publish("DataTopic",json.dumps(data) , qos=0, retain=False)

class SendDataActiveServer(threading.Thread):
     def __init__(self,threadID,name,queue):
         super(SendDataActiveServer,self).__init__()
         self.threadID = threadID
         self.name = name
         self.queue = queue

     def run(self):
         var active = false
         active = self.getItemFromQueue()

         while 1==1 :
           if active:
              send_data()
              time.sleep(1)
              active = self.getItemFromQueue()
           else:
              time.sleep(1)
              active = self.getItemFromQueue()

     def getItemFromQueue(self):
         try:
           return self.queue.get(block=False)
         except Queue.Empty:
           #quene.get()方法在队列中为空是返回异常，捕获异常返回false表示没有收到任何消息
           return false

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

    if d['cmd'] == "randnum":
       print("This is randnum cmd")
       d['randnum'] = 520.1314

    if d['cmd'] == "collect" && d['method'] == "set":
       print("This is collect cmd")
       d['result'] = "set successed."
       #param的值是true或false
       globalQueue.put(d['param'])


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

#开始独立线程用于主动发送数据
thread = SendDataActiveServer("Thread-1", "SendDataServerThread", globalQueue)
thread.start()

client.loop_forever()
