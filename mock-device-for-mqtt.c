/*
 * 1: 依赖库：cjson，eclipse paho mqtt.编译前请安装
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <cjson/cJSON.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "Mock-Device-Client"
#define DATA_TOPIC  "DataTopic"
#define CMDTOPIC    "CommandTopic"
#define RESPONSE_TOPIC "ResponseTopic"
#define USERNAME    "huaqiao"
#define PWD         "1234"
#define PAYLOAD     "{\"name\":\"mqtt-device-01\",\"randnum\":\"520.1314\"}"
#define QOS         0
#define TIMEOUT     10000L

static pthread_mutex_t mutex;

char *active = "false";

void publish(MQTTClient *client, char *payload, char *topic) {
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;

  pubmsg.payload = payload;
  pubmsg.payloadlen = strlen(payload);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  MQTTClient_publishMessage(*client, topic, &pubmsg, &token);
  printf("Waiting for up to %d seconds for publication of %s\n"
          "on topic %s for client with ClientID: %s\n",
          (int)(TIMEOUT/1000), payload, topic, CLIENTID);
  MQTTClient_waitForCompletion(*client, token, TIMEOUT);
  printf("Message with delivery token %d delivered\n", token);
}

void subscribe(MQTTClient *client) {
  MQTTClient_subscribe(*client, CMDTOPIC, QOS);
  printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", CMDTOPIC, CLIENTID, QOS);
}

void response_cmd(MQTTClient *client, char *resp_data) {
    publish(client,resp_data,RESPONSE_TOPIC);
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char *cmd;
    char *method;
    char *param;
    cJSON *item = NULL;
    cJSON *json = NULL;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %s\n",message->payload);

    json = cJSON_Parse(message->payload);
    item = cJSON_GetObjectItem(json,"cmd");

    cmd = item->valuestring;
    method = (cJSON_GetObjectItem(json,"method"))->valuestring;
    //处理ping命令
    if (strcmp(cmd,"ping") == 0) {
      cJSON_AddItemToObject(json, "ping", cJSON_CreateString("pong"));
    }

    //处理message命令
    if (strcmp(cmd,"message") == 0) {
      if (strcmp(method,"get") == 0) {
        cJSON_AddItemToObject(json, "message", cJSON_CreateString("Are you ok?"));
      } else {
       //param = cJSON_Print(cJSON_GetObjectItem(json,"param"));
       cJSON_AddItemToObject(json, "result", cJSON_CreateString("set success."));
      }
    }
    //处理randnum命令
    if (strcmp(cmd,"randnum") == 0) {
      cJSON_AddItemToObject(json, "randnum", cJSON_CreateString("520.1314"));
    }
    //处理collect命令
    if (strcmp(cmd,"collect") == 0) {
      if (strcmp(method,"get") == 0) {
        cJSON_AddItemToObject(json, "collect", cJSON_CreateString(active));
      } else {
        active = (cJSON_GetObjectItem(json,"param"))->valuestring;
      }
    }

    response_cmd((MQTTClient *)context,cJSON_PrintUnformatted(json));
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    // cJSON_Delete(json);
    // cJSON_Delete(item);
    return 1;
}

void *send_data_actively_server (void *context) {
  for (;;) {
    //pthread_mutex_lock(&mutex);
    if (strcmp(active,"true") == 0) {
      sleep(1);
      printf("send data actively from mock device.\n");
      publish((MQTTClient *)context,PAYLOAD,DATA_TOPIC);
    }
    //pthread_mutex_unlock(&mutex);
  }
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID,  MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.username = USERNAME;
    conn_opts.password = PWD;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    int rc;
    pthread_t thread_id;
    MQTTClient_setCallbacks(client, &client, NULL, on_message, NULL);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    } else {
      printf("Successful connection\n");
    }
    subscribe(&client);
    pthread_create(&thread_id, NULL, send_data_actively_server, (void *)&client);
    // pthread_join(thread_id, NULL);
    for (;;) {
      sleep(3);
    }
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
