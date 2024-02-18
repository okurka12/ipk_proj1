
# dont submit this kekl

# inspiration
# https://wiki.python.org/moin/UdpCommunication

import socket

UDP_IP = "127.0.0.1"  # localhost
UDP_PORT = 4567  # default IPK24-CHAT port

FAMILY = socket.AF_INET

# https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream
TYPE = socket.SOCK_DGRAM

MSG_TYPES = {
    0x00: "CONFIRM",
    0x01: "REPLY",
    0x02: "AUTH",
    0x03: "JOIN",
    0x04: "MSG",
    0xFE: "ERR",
    0xFF: "BYE",
}

MSG_INV_TYPES = {value: key for key, value in MSG_TYPES.items()}



def no_lf(s: str) -> str:
    r"""replaces (CR)LF with \n"""
    o = s.replace("\r", "")
    return o.replace("\n", "\\n")


def recv_loop(sock: socket.socket) -> None:

    while True:

        # wait for the message
        response = sock.recv(2048)

        # parse message
        msgtype = int(response[0])
        msgtype = MSG_TYPES[msgtype] if msgtype in MSG_TYPES else "unknown"
        msgid = int.from_bytes(response[1:3], byteorder="big")

        # ignore CONFIRM sent by self
        if msgtype == "CONFIRM":
            continue

        # print on stdout
        print(f"TYPE: {msgtype}")
        print(f"ID: {msgid}")
        print(f"'{no_lf(response[3:].decode('utf-8'))}'")

        # send confirm
        reply = bytearray(3)
        reply[0] = MSG_INV_TYPES["CONFIRM"]
        reply[1] = response[1]
        reply[2] = response[2]
        sock.sendto(reply, (UDP_IP, UDP_PORT))
        print("sent confirm...")





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
