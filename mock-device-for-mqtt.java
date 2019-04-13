package mockdevice;

import java.util.Map;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;
import org.eclipse.paho.client.mqttv3.IMqttMessageListener;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

public class MockDevice4Mqtt {

	private static String BROKER_HOST_ADDR = "tcp://192.168.56.4:1883";
	private static String CLIENTID         = "Mock-Device-ID";
	private static String RESP_CLIENTID    = "Mock-Device-Response-ID";
	private static String ACTIVE_CLIENTID  = "Mock-Device-Active-ID";
	private static int QOS                 = 0;

	private static String USERNAME      	 = "huaqiao";
	private static String PWD          	 = "1234";
	private static String CMD_TOPIC		 = "CommandTopic";
	private static String RESPONSE_TOPIC  = "ResponseTopic";
	private static String DATA_TOPIC      = "DataTopic";
	private static String PAYLOAD         = "{\"name\":\"mqtt-device-01\",\"randnum\":\"520.1314\"}";

	private MqttClient client = null;
	private ObjectMapper JSONmapper = new ObjectMapper();
	private String active = "false";
	private final Queue<String> globalQueue = new LinkedBlockingQueue<String>(5);

	public static void main(String[] args) {
		new MockDevice4Mqtt();
		for(;;){
	        try {
	          Thread.sleep(1000);
	        } catch (InterruptedException e) {
	          e.printStackTrace();
	        }
	    }
	}

	public MockDevice4Mqtt() {
		initClient();
		new Thread(new Runnable() {
			@Override
			public void run() {
				MqttClient dataClient = null;
				try {
					dataClient = new MqttClient(BROKER_HOST_ADDR, ACTIVE_CLIENTID,new MemoryPersistence());
					MqttConnectOptions connOpts = new MqttConnectOptions();
					connOpts.setUserName(USERNAME);
					connOpts.setPassword(PWD.toCharArray());
					connOpts.setCleanSession(true);
					dataClient.connect(connOpts);
					System.out.println("Ready to send actively.");
				} catch (MqttException e) {
					System.out.println("Can't connect when send actively.");
					e.printStackTrace();
				}

				for (;;) {
					Object item = globalQueue.poll();
					if (item != null) {
						active = (String)item;
					}
					if ("true".equals(active)) {
						try {
							Thread.sleep(1000);
							dataClient.publish(DATA_TOPIC, PAYLOAD.getBytes(), QOS, false);
							System.out.println("send data actively : " + PAYLOAD);
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				}
			}
		}).start();
	}

	public void initClient() {
		try {
			client = new MqttClient(BROKER_HOST_ADDR, CLIENTID,new MemoryPersistence());
			MqttConnectOptions connOpts = new MqttConnectOptions();
			connOpts.setUserName(USERNAME);
			connOpts.setPassword(PWD.toCharArray());
			connOpts.setCleanSession(false);
			client.connect(connOpts);
		} catch (MqttException e) {
			System.out.println("can't connect to broker！");
			e.printStackTrace();
		}
		System.out.println("connect success！");

		try {
			client.subscribe(CMD_TOPIC,new IMqttMessageListener() {
				@Override
				public void messageArrived(String topic, MqttMessage message) throws Exception {
					String jsonStr = new String(message.getPayload());
					System.out.println("Receive cmd : " + jsonStr);
					commandHandler(message.getPayload());
				}
			});
		} catch (MqttException e) {
			e.printStackTrace();
		}
		System.out.println("Start subscribe " + CMD_TOPIC + " topic .");
	}

	@SuppressWarnings("unchecked")
	public void commandHandler(byte[] cmdPaylod) {
		Map<String,String> cmdMap = null;
		try {
			cmdMap = JSONmapper.readValue(cmdPaylod, Map.class);
		} catch (Exception e) {
			System.out.println("Json convert failed." );
		}
		String cmd = cmdMap.get("cmd");
		String method = cmdMap.get("method");
		System.out.println(cmd + " : " + method );

		switch (cmd) {
		case "ping":
			cmdMap.put(cmd, "pong");
			break;
		case "randnum":
			cmdMap.put(cmd, "520.1314");
			break;
		case "message":
			if ("get".equals(method)) {
				cmdMap.put(cmd, "Are you ok?");
			} else {
				cmdMap.put("result", "set success.");
			}
			break;
		case "collect":
			if ("get".equals(method)) {
				cmdMap.put(cmd, active);
			} else {
				cmdMap.put("result", "set success.");
				globalQueue.add(cmdMap.get("param"));
			}
			break;
		}
		try {
			responseCommand(RESPONSE_TOPIC,JSONmapper.writeValueAsString(cmdMap));
		} catch (JsonProcessingException e) {
			e.printStackTrace();
		}
	}

	public void responseCommand(String topic,String payload) {
		MqttClient respClient = null;
		try {
			respClient = new MqttClient(BROKER_HOST_ADDR, RESP_CLIENTID,new MemoryPersistence());
			MqttConnectOptions connOpts = new MqttConnectOptions();
			connOpts.setUserName(USERNAME);
			connOpts.setPassword(PWD.toCharArray());
			connOpts.setCleanSession(true);
			respClient.connect(connOpts);
			respClient.publish(topic, payload.getBytes(), QOS, false);
			System.out.println("Response Cmd :" + payload);
			respClient.disconnect();
		} catch (Exception e) {
			System.out.println("can't publish msg success.");
			e.printStackTrace();
			try {
				respClient.disconnect();
			} catch (MqttException e1) {
				System.out.println("can't disconnect client.");
				e1.printStackTrace();
			}
		}
	}
}
