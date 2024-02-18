
# dont submit this kekl

import socket

UDP_IP = "127.0.0.1"
UDP_PORT = 4567  # default IPK24-CHAT port

FAMILY = socket.AF_INET

# https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream
TYPE = socket.SOCK_DGRAM



def main():
    sock = socket.socket(FAMILY, TYPE)
    sock.bind(UDP_IP, UDP_PORT)
    data = sock.recv(2048)
    print(f"received data: {data.decode("utf-8")}")
    sock.send("data received".encode("utf-8"))
    print("exiting...")
