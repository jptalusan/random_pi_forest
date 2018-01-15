import paho.mqtt.client as mqtt
import time

def on_message(client, userdata, message):
    print "Message received: "  + message.payload

def on_subscribe(mqttc, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected
        Connected = True
    else:
        print("Connection failed")

Connected=False
#Client(client_id="redundancy")
client = mqtt.Client("python",clean_session=True, userdata=None, protocol=MQTTv311, transport="tcp") 
client.on_connect=on_connect
client.on_mesage=on_message
client.on_subscribe=on_subscribe
host="iot.eclipse.org"
port=1883

client.connect(host, port)

client.loop_start()

while Connected != True:
    time.sleep(0.1)

client.subscribe("hello")

try:
    while True:
        time.sleep(1)

except KeyboardInterrupt:
    print("Exiting")
    client.disconnect()
    client.loop_stop()
