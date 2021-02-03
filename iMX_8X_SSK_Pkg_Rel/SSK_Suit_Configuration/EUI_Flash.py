import logging
import time
import argparse
import json
import sys
import fcntl, socket, struct
import os
file_eui64 = open("EUI.bin","w+")
def get_mac(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', ifname[:15]))
    return ':'.join(['%02x' % ord(char) for char in info[18:24]])

os.system("echo  ")
os.system("echo ---> Getting Wifi-MAC Address <--- ")

mac = get_mac('wlan0')
time.sleep(2)
first_3 = mac[0:8]
last_3 = mac[9:17]
print("\n")
EUI = first_3+":00:01:"+last_3
os.system("echo  ")
os.system("echo ---> Converted  Wifi-MAC Address to EUI64 Unique-Format <--- ")

os.system("echo  ")
print(EUI)
file_eui64.write(EUI)
file_eui64.close()
time.sleep(2)
os.system("echo  ")
os.system("echo ---> EUI64 Unique-Format Writing To TPM Protected NV Area <--- ")
os.system("tpm2_nvdefine -x 0x1500100 -a o -s 23 -b 0x2000A ")
time.sleep(5)
os.system("tpm2_nvwrite -x 0x1500100 -a o EUI.bin")
time.sleep(5)





