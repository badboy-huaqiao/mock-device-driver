/*
 * 1: 依赖库：cjon，eclipse paho mqtt.编译前请安装
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include <MQTTAsync.h>

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

void on_send_success(void* context, MQTTAsync_successData* response) {
   printf("Message with token value %d delivery confirmed\n", response->token);
}

void publish(MQTTAsync *client, char *payload, char *topic) {
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    int rc;
    opts.onSuccess = on_send_success;
    opts.context = client;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    opts.context = client;
    //pthread_mutex_lock(&mutex);
    if ((rc = MQTTAsync_sendMessage(*client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start sendMessage, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    //pthread_mutex_unlock(&mutex);
    printf("Waiting for up to %d seconds for publication of %s\n",(int)(TIMEOUT/1000), payload);
}

void response_cmd(void *context, char *resp_data) {
    publish(context,resp_data,RESPONSE_TOPIC);
}

int on_message(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
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

    response_cmd(context,cJSON_PrintUnformatted(json));

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    // cJSON_Delete(json);
    // cJSON_Delete(item);
    return 1;
}

void on_connect(void* context, MQTTAsync_successData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;
    printf("Successful connection\n");
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", CMDTOPIC, CLIENTID, QOS);
    opts.context = client;

    if ((rc = MQTTAsync_subscribe(client, CMDTOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
}

void *send_data_actively_server(void *context) {
  for (;;) {
    if (strcmp(active,"true") == 0) {
      sleep(1);
      printf("send data actively from mock device.\n");
      publish((MQTTAsync *)context,PAYLOAD,DATA_TOPIC);
    }
  }
}

int main(int argc, char* argv[]) {
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    //MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_token token;
    int rc;
    pthread_t thread_id;

    MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, &client, NULL, on_message, NULL);
    conn_opts.MQTTVersion = MQTTVERSION_DEFAULT;
    conn_opts.username = USERNAME;
    conn_opts.password = PWD;
    conn_opts.keepAliveInterval = 2000;
    conn_opts.cleansession = 0;
    conn_opts.onSuccess = on_connect;
    conn_opts.context = client;
    conn_opts.automaticReconnect = 1;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        exit(EXIT_FAILURE);

    }

    // disc_opts.onSuccess = onDisconnect;
    // if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
    //        printf("Failed to start disconnect, return code %d\n", rc);
    //        exit(EXIT_FAILURE);
    // }

    pthread_create(&thread_id, NULL, send_data_actively_server, &client);
    //pthread_join(thread_id, NULL);

    for(;;) {
       sleep(3);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTAsync_destroy(&client);
    return rc;
  }
