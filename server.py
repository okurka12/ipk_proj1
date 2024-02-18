
# dont submit this kekl

# inspiration
# https://wiki.python.org/moin/UdpCommunication

import socket

UDP_IP = "127.0.0.1"  # localhost
UDP_PORT = 4567  # default IPK24-CHAT port

FAMILY = socket.AF_INET

# https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream
TYPE = socket.SOCK_DGRAM

MSG_TYPES = [
    "CONFIRM",
    "REPLY",
    "AUTH",
    "JOIN",
    "MSG",
    "ERR",
    "BYE",
    "unknown"
]


def no_lf(s: str) -> str:
    r"""replaces (CR)LF with \n"""
    o = s.replace("\r", "")
    return o.replace("\n", "\\n")


def recv_loop(sock: socket.socket) -> None:

    while True:
        # wait for the message
        response = sock.recv(2048)
        message_type = int(response[0])
        message_id = int.from_bytes(response[1:3], byteorder="big")
        response = response[3:]

        if message_type > len(MSG_TYPES) - 2:
            message_type = len(MSG_TYPES) -1
        print(f"TYPE: {MSG_TYPES[message_type]}")
        print(f"ID: {message_id}")
        print(f"'{no_lf(response.decode('utf-8'))}'")




def main():

    # create socket and bind
    sock = socket.socket(FAMILY, TYPE)
    sock.bind((UDP_IP, UDP_PORT))

    try:
        recv_loop(sock)
    except KeyboardInterrupt:
        print()


    print("exiting...")


if __name__ == "__main__":
    main()
