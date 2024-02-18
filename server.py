
# dont submit this kekl

# inspiration
# https://wiki.python.org/moin/UdpCommunication

import socket

UDP_IP = "127.0.0.1"  # localhost
UDP_PORT = 4567  # default IPK24-CHAT port

FAMILY = socket.AF_INET

# https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream
TYPE = socket.SOCK_DGRAM


def no_lf(s: str) -> str:
    r"""replaces (CR)LF with \n"""
    o = s.replace("\r", "")
    return o.replace("\n", "\\n")



def main():

    # create socket and bind
    sock = socket.socket(FAMILY, TYPE)
    sock.bind((UDP_IP, UDP_PORT))

    # wait for the message
    response = sock.recv(2048)
    response = response.decode("utf-8")

    print(f"received data: '{no_lf(response)}'")
    print("exiting...")


if __name__ == "__main__":
    main()
