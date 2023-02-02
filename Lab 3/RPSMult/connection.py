import paho.mqtt.client as mqtt
import numpy as np
from datetime import datetime

def gen_client():
  def on_connect(client, userdata, flags, rc):
    # print("Connection returned result: " + str(rc))
    client.subscribe("ece180d/test/A412", qos=1)
  def on_disconnect(client, userdata, rc):
    if rc != 0:
      print('Unexpected Disconnect')
    else:
      print('Expected Disconnect')

  client = mqtt.Client()
  client.on_connect = on_connect
  client.on_disconnect = on_disconnect
  #client.on_message = on_message
  # client.connect_async('mqtt.eclipseprojects.io')
  # client.loop_start()
  #print('Publishing...')
  #client.publish("ece180d/test/A412", 'P2,50,50', qos=1)
  #client.loop_stop()
  #client.disconnect()

  return client