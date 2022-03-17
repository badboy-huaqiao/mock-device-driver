#!/usr/bin/python
# -*- coding: UTF-8 -*-

#version: python 2.7
#如果是python3 需要修改部分依赖包或者语句语法
#用于模拟一个基于mqtt协议传输虚拟设备驱动

import paho.mqtt.client as mqtt
import json
import time
import Queue
import threading,random

BROKER_HOST_ADDR   = "localhost"
BROKER_HOST_PORT   = 1883
USERNAME    = "huaqiaoz" #非安全模式忽略用户名秘密
PWD         = "1234" #非安全模式忽略用户名秘密
#cmd topic本质上就是你的设备监听的topic，
#也是在UI上添加device的时候，地址中所填数据，和用户名密码等一起组成当前设备的唯一标识。
CMD_TOPIC   = "CommandTopic"
RESPONSE_TOPIC = "ResponseTopic"
DATA_TOPIC  = "DataTopic"

globalQueue = Queue.Queue()

def gen():
       return round(random.uniform(0, 50),2)

def send_data():
    #java版本, name的值为添加的设备名
    #data = {"randnum":520.1314,"name":"mqtt-device-01"}

    #go版本, name的值为添加的设备名（required）, go版本的区别是必须带上cmd字段
    # data = {"randnum":520.1314,"name":"mqtt-device-01","cmd":"randnum","method":"get"}
    
    #v2.1.0版本数据格式
    #数据格式 data = {"randnum":520.1314,"name":"mqtt-device-01","cmd":"randnum","method":"get"}
    data = {"randnum":gen(),"name":"mqtt-device-01","cmd":"randnum", "method":"get"}
    print("sending data actively! " + json.dumps(data))
    client.publish(DATA_TOPIC,json.dumps(data) , qos=0, retain=False)

class SendDataActiveServer(threading.Thread):
    def __init__(self,threadID,name,queue):
        super(SendDataActiveServer,self).__init__()
        self.threadID = threadID
        self.name = name
        self.queue = queue
        self.active = False

    def run(self):
        while 1==1 :
          #v2.1.0 接收到的bool值自动自动转换为python能识别的bool值，无须二次转换
          if self.active:
             send_data()
             time.sleep(1)
             self.getItemFromQueue()
          else:
             time.sleep(1)
             self.getItemFromQueue()

    def getItemFromQueue(self):
        try:
          if self.queue.get(block=False):
             self.active = True
          else:
             self.active = False
        except Queue.Empty:
          #quene.get()方法在队列中为空是返回异常，捕获异常什么都不做，保持active原状
          time.sleep(0.1)

#当接收到命令，响应命令
#v2.1.0 get命令格式： {"cmd":"ping","method":"get","uuid":"f46ae5c7-2a08-4f56-a38b-d9e122d255d0"}
#v2.1.0 set命令格式 {"cmd":"ping","ping":"ping_set_value","method":"set","uuid":"95d559ad-7140-4d62-8dd8-6465c37fec10"}
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload)+'\n')
    d = json.loads(msg.payload)

    if d['cmd'] == "message":
       print("This is message cmd")
       if d['method'] == "get":
          d['message'] = "Are you ok?"
       elif d['method'] == "set":
          d['result'] = "set successed."

    if d['cmd'] == "ping":
       print("This is ping cmd")
       d['ping'] = "pong"

    if d['cmd'] == "randnum":
       print("This is randnum cmd")
       d['randnum'] = gen()

    if d['cmd'] == "collect" and d['method'] == "set":
       print("This is collect set cmd")
       d['result'] = "set successed."
       #param的值是true或false,且是字符串类型, 
       #globalQueue.put(d['param'])
       #v2.1.0命令格式中移除了param属性，使用device resource name为实际参数名
       globalQueue.put(d['collect'])
    elif d['cmd'] == "collect" and d['method'] == "get":
       print("This is collect get cmd")
       d['collect'] = thread.active

    print(json.dumps(d))
    client.publish(RESPONSE_TOPIC, json.dumps(d))

def on_connect(client, userdata, flags, rc):
    print("Connected success with result code "+str(rc))
    #监听命令
    client.subscribe(CMD_TOPIC)

client = mqtt.Client()
client.username_pw_set(USERNAME, PWD)
client.on_message = on_message
client.on_connect = on_connect

client.connect(BROKER_HOST_ADDR, BROKER_HOST_PORT, 60)

#开始独立线程用于主动发送数据
thread = SendDataActiveServer("Thread-1", "SendDataServerThread", globalQueue)
thread.setDaemon(True)
thread.start()

client.loop_forever()
