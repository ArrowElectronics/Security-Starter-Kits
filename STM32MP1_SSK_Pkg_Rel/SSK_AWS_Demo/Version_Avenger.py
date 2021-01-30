
from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import logging
import time
import argparse
import json
import sys
import fcntl, socket, struct
import os
file_eui64 = open("EUI_original.bin","w+") #This file will capture the EUI64 Unique ID from TPM2.0
file_linux = open("Firmware.bin","w+")     #This file will capture the latest Linux firmware version
file_ggc = open("ggc_version.bin","w+")    #This file will capture the latest GGC firmware version
os.system("tpm2_nvread -x 0x1500100 -a 0x40000001 -s 23 >> EUI_original.bin")
os.system("uname -a >> Firmware.bin")
os.system("/greengrass/ggc/core/greengrassd -v >> ggc_version.bin")
time.sleep(3)

def get_mac(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', ifname[:15]))
    return ':'.join(['%02x' % ord(char) for char in info[18:24]])

mac=file_eui64.read() #Values read of EUI64 will be stored in this Variable
version=file_linux.read() #Values read of Version will be stored in this Variable
ggc=file_ggc.read() #Values read of GGC Version will be stored in this Variable
file_linux.close()
file_ggc.close()
file_eui64.close()
version_firmware = version[0:27]
version_ggc = ggc[0:23]
AllowedActions = ['both', 'publish', 'subscribe']

def parse_json_data(json_data):
    print("prase json data")
    json_obj = json.loads(json_data)
    parsed_data = json_obj['message']
    print(parsed_data)
    return parsed_data

def create_json():
	Jdata = {}
        Jdata['device_type']  = "2"
        Jdata['greengrass']  = version_ggc
        Jdata['EUI64']   = mac 
        Jdata['firmware']   = version_firmware
        return Jdata


# Custom MQTT message callback
def customCallback(client, userdata, message):
    print("Received a new message: ")
    print(message.payload)
    print(mac)
    if message.payload == '2':
		json_data = create_json()
		messageJson = json.dumps(json_data)
        	myAWSIoTMQTTClient.publish(pubtopic, messageJson , 0)
    print("--------------\n\n")


# Read in command-line parameters
parser = argparse.ArgumentParser()
parser.add_argument("-e", "--endpoint", action="store", required=True, dest="host", help="Your AWS IoT custom endpoint")
parser.add_argument("-r", "--rootCA", action="store", required=True, dest="rootCAPath", help="Root CA file path")
parser.add_argument("-c", "--cert", action="store", dest="certificatePath", help="Certificate file path")
parser.add_argument("-k", "--key", action="store", dest="privateKeyPath", help="Private key file path")
parser.add_argument("-p", "--port", action="store", dest="port", type=int, help="Port number override")
parser.add_argument("-w", "--websocket", action="store_true", dest="useWebsocket", default=False,
                    help="Use MQTT over WebSocket")
parser.add_argument("-n", "--thingName", action="store", dest="thingName", default="Bot", help="Targeted thing name")
parser.add_argument("-id", "--clientId", action="store", dest="clientId", default="GG_Switch",
                    help="Targeted client id")
parser.add_argument("-t", "--topic", action="store", dest="topic", default="sdk/test/Python", help="Targeted topic")
parser.add_argument("-m", "--mode", action="store", dest="mode", default="both",
                    help="Operation modes: %s"%str(AllowedActions))
parser.add_argument("-M", "--message", action="store", dest="message", default="Hello World!",
                    help="Message to publish")

args = parser.parse_args()
host = args.host
rootCAPath = args.rootCAPath
certificatePath = args.certificatePath
privateKeyPath = args.privateKeyPath
port = args.port
useWebsocket = args.useWebsocket
clientId = args.clientId
thingName = args.thingName
pubtopic = "sub/avg/version"
subtopic = "pub/device/version"
topic = args.topic

if args.mode not in AllowedActions:
    parser.error("Unknown --mode option %s. Must be one of %s" % (args.mode, str(AllowedActions)))
    exit(2)

if args.useWebsocket and args.certificatePath and args.privateKeyPath:
    parser.error("X.509 cert authentication and WebSocket are mutual exclusive. Please pick one.")
    exit(2)

if not args.useWebsocket and (not args.certificatePath or not args.privateKeyPath):
    parser.error("Missing credentials for authentication.")
    exit(2)

# Port defaults
if args.useWebsocket and not args.port:  # When no port override for WebSocket, default to 443
    port = 443
if not args.useWebsocket and not args.port:  # When no port override for non-WebSocket, default to 8883
    port = 8883

# Configure logging
logger = logging.getLogger("AWSIoTPythonSDK.core")
logger.setLevel(logging.DEBUG)
streamHandler = logging.StreamHandler()
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
streamHandler.setFormatter(formatter)
logger.addHandler(streamHandler)

# Init AWSIoTMQTTClient
myAWSIoTMQTTClient = None
if useWebsocket:
    myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId, useWebsocket=True)
    myAWSIoTMQTTClient.configureEndpoint(host, port)
    myAWSIoTMQTTClient.configureCredentials(rootCAPath)
else:
    myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId)
    myAWSIoTMQTTClient.configureEndpoint(host, port)
    myAWSIoTMQTTClient.configureCredentials(rootCAPath, privateKeyPath, certificatePath)

# AWSIoTMQTTClient connection configuration
myAWSIoTMQTTClient.configureAutoReconnectBackoffTime(1, 32, 20)
myAWSIoTMQTTClient.configureOfflinePublishQueueing(-1)  # Infinite offline Publish queueing
myAWSIoTMQTTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
myAWSIoTMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
myAWSIoTMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec

# Connect and subscribe to AWS IoT
try:
	myAWSIoTMQTTClient.connect()
except:
	print("mqtt error\n")
	sys.exit(-1)
if args.mode == 'both' or args.mode == 'subscribe':
    myAWSIoTMQTTClient.subscribe(subtopic, 0, customCallback)
time.sleep(2)

# Publish to the same topic in a loop forever
loopCount = 0
while True:
    loopCount += 1
    time.sleep(1)
