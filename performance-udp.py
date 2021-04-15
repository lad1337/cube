#!/usr/bin/python3
import os
import psutil
import socket
import time

TARGET_IP = "10.0.13.50"
TARGET_PORT = 1234
SLEEP = 1
LOAD_INDEX = 1  # 0=load1, 1=load5, 2=load15


while True:
    load = os.getloadavg()[LOAD_INDEX]
    net_io = psutil.net_io_counters(pernic=False)
    download = net_io.bytes_recv
    upload = net_io.bytes_sent

    time.sleep(SLEEP)

    net_io = psutil.net_io_counters(pernic=False)
    download = net_io.bytes_recv - download
    upload = net_io.bytes_sent - upload

    out = ",".join(map(lambda x: str(round(x, 3)), [load, download, upload]))
    print(out)
    socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(
        out.encode("utf-8"), (TARGET_IP, TARGET_PORT)
    )
