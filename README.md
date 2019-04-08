# mock-device-driver

## 模拟一个基于mqtt协议通信的虚拟device，用于演示EdgeX Foundry的device-mqtt设备微服务的通信演示
## 假设这个python脚本就是你的物理设备上的驱动，用于监听命令，响应命令，且具备主动发送数据的能力。
## 其中
### DataTopic是设备主动发送数据的topic，这个topic是device-mqtt微服务监听的
### CommandTopic是设备监听即将到来的命令topic，填写设备地址的时候，就是填写的这个topic，可自定义
### ResponseTopic是响应命令topic，也就是device-mqtt微服务监听的topic

## 以上DataTopic，ResponseTopic都可以通过修改device-mqtt设备微服务源码的配置文件而自定义


