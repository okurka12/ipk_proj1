
# dont submit this kekl

# inspiration
# https://wiki.python.org/moin/UdpCommunication

import socket
import time
import pdb


# what address to listen on
# BIND_IP = "127.0.0.1"  # localhost (loopback only)
BIND_IP = "0.0.0.0"  # listen on every available network interface
UDP_PORT = 4567  # default IPK24-CHAT port

FAMILY = socket.AF_INET

# https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream
TYPE = socket.SOCK_DGRAM

MSG_TYPES: dict[str] = {
    0x00: "CONFIRM",
    0x01: "REPLY",
    0x02: "AUTH",
    0x03: "JOIN",
    0x04: "MSG",
    0xFE: "ERR",
    0xFF: "BYE",
}

MSG_INV_TYPES = {value: key for key, value in MSG_TYPES.items()}


class Message:
    def __init__(self, msg: bytes) -> None:
        # parse message header
        self.type = int(msg[0])
        self.type = \
            MSG_TYPES[self.type] if self.type in MSG_TYPES else "unknown"
        self.id = int.from_bytes(msg[1:3], byteorder="big")

        # message type
        msgt = self.type

        if   msgt == "CONFIRM":
            pass
        elif msgt == "REPLY":
            self.result = int(msg[3])
            self.ref_msgid = int.from_bytes(msg[4:6], byteorder="big")
        elif msgt == "AUTH":
            pass
        elif msgt == "JOIN":
            pass
        elif msgt == "MSG":
            self.messagecontents = str_from_bytes(3, msg)
        elif msgt == "ERR":
            pass
        elif msgt == "BYE":
            pass

        # # REPLY
        # self.result = 0
        # self.ref_msgid = 0
        # self.messagecontents = ""

        # # AUTH
        # self.username = ""
        # self.displayname = ""
        # self.secret = ""

        # # JOIN
        # self.channel_id = ""
        # self.displayname = ""

        # # MSG
        # self.displayname = ""
        # self.messagecontents = ""

        # # ERR
        # self.displayname = ""
        # self.messagecontents = ""

        # BYE
        pass

    def __repr__(self) -> str:
        output = ""
        if self.type == "MSG":
            output += f"TYPE: {self.type}\n"
            output += f"ID: {self.id}\n"
            output += f"'{no_lf(self.messagecontents)}'"
        else:
            output += f"TYPE: {self.type}\n ID: {self.id}"
        return output



def str_from_bytes(startpos: int, b: bytes) -> str:
    """
    reads a null terminated string beginning at `startpos` in `b`.
    returns the string
    """
    output = ""
    i = startpos
    while b[i] != 0 and i < len(b) - 1:
        output += chr(b[i])
        i += 1
    return output



def no_lf(s: str) -> str:
    r"""replaces (CR)LF with \n"""
    o = s.replace("\r", "")
    return o.replace("\n", "\\n")


def recv_loop(sock: socket.socket) -> None:

    while True:

        # wait for the message
        response, retaddr = sock.recvfrom(2048)

        msg = Message(response)

        # ignore CONFIRM sent by self
        if msg.type == "CONFIRM":
            continue

        # print on stdout
        print(f"MESSAGE from {retaddr[0]}:")
        print(msg)
        print()

        # sleep for 50 ms
        time.sleep(0.1)

        # send confirm
        reply = bytearray(3)
        reply[0] = MSG_INV_TYPES["CONFIRM"]
        reply[1] = response[1]
        reply[2] = response[2]
        sock.sendto(reply, retaddr)





def main():

    # create socket and bind
    sock = socket.socket(FAMILY, TYPE)
    sock.bind((BIND_IP, UDP_PORT))
    print(f"started server on {BIND_IP} port {UDP_PORT}")

    try:
        recv_loop(sock)
    except KeyboardInterrupt:
        print()


    print("exiting...")


if __name__ == "__main__":
    main()
