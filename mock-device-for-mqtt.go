/*
 * 依赖库：go get github.com/eclipse/paho.mqtt.golang
 */
package main

import (
	"encoding/json"
	MQTT "github.com/eclipse/paho.mqtt.golang"
	"log"
	"time"
)

const (
	BROKER_HOST    = "tcp://192.168.56.4:1883"
	USERNAME       = "huaqiao"
	PWD            = "1234"
	CMD_TOPIC      = "CommandTopic"
	RESPONSE_TOPIC = "ResponseTopic"
	DATA_TOPIC     = "DataTopic"
	PAYLOAD        = "{\"name\":\"mqtt-device-01\",\"randnum\":\"520.1314\"}"

	RESP_CLIENTID = "Mock-Device-Response-ID"
	CLIENTID      = "Mock-Device-ID"
)

var active = "false"
var msgCh = make(chan string, 1)

//var msgRecHandler MQTT.MessageHandler =

func main() {
	opts := MQTT.NewClientOptions().AddBroker(BROKER_HOST)
	opts.SetUsername(USERNAME)
	opts.SetPassword(PWD)
	opts.SetClientID(CLIENTID)
	opts.OnConnect = MQTT.OnConnectHandler(onConnnect)

	client := MQTT.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Println("can't connect to broker.")
		panic(token.Error())
	}

	go sendDataActiveServer(msgCh, client)

	for {
		time.Sleep(3 * time.Second)
	}
}

func onConnnect(client MQTT.Client) {
	log.Println("Connect to broker successed. ")
	if t := client.Subscribe(CMD_TOPIC, 0, MQTT.MessageHandler(msgRecHandler)); t.Wait() && t.Error() != nil {
		log.Println("Can't not subscribe " + CMD_TOPIC + " topic.")
		panic(t.Error())
	}
	log.Println("Start subscribe " + CMD_TOPIC + " topic.")
}

func msgRecHandler(client MQTT.Client, msg MQTT.Message) {
	log.Printf("Recv msg : %s\n", msg.Payload())
	cmdMap := make(map[string]string)
	json.Unmarshal(msg.Payload(), &cmdMap)

	cmd := cmdMap["cmd"]
	method := cmdMap["method"]

	switch cmd {
	case "ping":
		cmdMap["ping"] = "pong"
	case "randnum":
		cmdMap["randnum"] = "520.1314"
	case "message":
		if method == "get" {
			cmdMap["message"] = "Are you ok?"
		} else {
			cmdMap["result"] = "set successed."
		}
	case "collect":
		if method == "get" {
			cmdMap["collect"] = active
		} else {
			cmdMap["result"] = "set successed."
			active = cmdMap["param"]
		}
	}
	respMsg, err := json.Marshal(cmdMap)
	if err != nil {
		log.Println(err)
	}
	token := client.Publish(RESPONSE_TOPIC, 0, false, respMsg)
	token.Wait()
	log.Println("Response cmd : " + string(respMsg))
}

func sendDataActiveServer(ch <-chan string, client MQTT.Client) {
	for {
		select {
		case msg, ok := <-ch:
			if ok {
				active = msg
			}
		default:
			time.Sleep(100 * time.Millisecond)
		}

		if active == "true" {
			log.Println("send data actively from mock device.")
			log.Println("         " + PAYLOAD)

			token := client.Publish(DATA_TOPIC, 0, false, PAYLOAD)
			token.Wait()
			time.Sleep(1 * time.Second)
		}
	}
}
