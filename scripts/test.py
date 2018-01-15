import paho.mqtt.client as mqtt
import signal
import sys
import subprocess

def signal_handler(signal, frame):
    print("Exiting2222222222222222222222")
    client.disconnect()
    client.loop_stop()
    sys.exit(0)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("master/lastWill/#")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))
    p = subprocess.Popen(["pgrep", "master"], stdout=subprocess.PIPE)
    
    out = p.communicate()[0].rstrip()
    print ("$" + out + "$")
    subprocess.Popen(["kill", out])
    
    #np = subprocess.call("./master_node", cwd="~/random_pi_forest/distRF/bin/master_node/")
    subprocess.Popen('cd ~/random_pi_forest/distRF/bin/master_node; ./master_node >/dev/null 2>&1', shell=True)
    '''
    while True:
        line = p.stdout.readline()
        if line != '':
            #the real code does filtering here
            print "test:", line.rstrip()
        else:
            break
    '''

signal.signal(signal.SIGINT, signal_handler)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("163.221.68.211", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()

