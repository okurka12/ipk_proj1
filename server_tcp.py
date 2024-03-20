########################
# IPK24CHAT TCP server #
#   o k u r k a  1 2   #
########################

import socket
import re
from time import sleep
import datetime as dt
from typing import Set
# from server_udp import no_lf

# when printing messages, print the original, not parsed form
PRINT_RAW = True

# send successful REPLY replies to the AUTH messages
AUTH_SUCCES = True

# print bloat?
VERBOSE = False

BIND_IP = "0.0.0.0"
BIND_PORT = 4567
FAMILY = socket.AF_INET
SOCKTYPE = socket.SOCK_STREAM

MTYPE_AUTH = "AUTH"

RE_USERNAME = r"((?:[A-z]|[0-9]|-){1,20})"
RE_DISPLAYNAME = r"([!-~]{1,20})"
RE_SECRET = r"((?:[A-z]|[0-9]|-){1,128})"

AUTH_PATTERN = r"AUTH " + RE_USERNAME + r" AS " + RE_DISPLAYNAME  + \
r" USING " + RE_SECRET + r"\r\n"


class Message:
    def __init__(self, data: bytes) -> None:
        self.type = "unknown"
        self.raw_data = data
        text = data.decode("utf-8")

        if text.lower().startswith("auth"):
            match_obj = re.match(AUTH_PATTERN, text)
            if match_obj is None:
                # todo: send err?
                tprint(f"couldn't parse {data}")
                return
            self.type = MTYPE_AUTH
            self.username = match_obj[1]
            self.displayname = match_obj[2]
            self.secret = match_obj[3]

    def __repr__(self) -> str:
        pre = ""  # prefix
        suf = ""  # suffix
        delim = "\n"  # delimiter

        output = pre
        if self.username is not None:
            output += f"Username: {self.username}" + delim
        if self.displayname is not None:
            output += f"Display Name: {self.displayname}" + delim
        if self.secret is not None:
            output += f"Secret: {self.secret}" + delim

        return output + suf



class Connection:
    def __init__(self, sock: socket.socket, addr: tuple[str, int]) -> None:
        sock.setblocking(False)
        self.sock = sock
        self.addr = addr[0]
        self.port = addr[1]
        self.active = True
    def __repr__(self) -> str:
        return f"{self.addr}:{self.port}"
    def set_inactive(self) -> None:
        self.active = False

connections: Set[Connection] = set()


def print_time(lf=False):
    print(dt.datetime.now(), end="\n" if lf else ": ")


def tprint(*args, **kwargs):
    print_time()
    print(*args, **kwargs)


def clean_connections():
    """
    remove inactive connections from global `connections`
    """
    global connections
    inactive_connections = {conn for conn in connections if not conn.active}
    connections = connections.difference(inactive_connections)


def print_connections() -> None:
    tprint(f"there are {len(connections)} clients connected: {connections}")


def try_recv(conn: Connection):
    """
    try to call recv on `conn` (non-blocking socket)
    if there is no data, do nothing
    """
    sock = conn.sock
    try:
        data = sock.recv(65536)
        if (len(data) == 0):
            conn.set_inactive()
            tprint(f"{conn} disconnected")
            return

        # parse the data
        msg = Message(data)

        # print the message
        tprint(f"\n{len(data)} B from {conn}:")
        if PRINT_RAW:
            print(msg.raw_data)
        print(msg)

    except BlockingIOError:
        pass


def accept_loop() -> None:
    sock = socket.socket(FAMILY, SOCKTYPE)
    sock.bind((BIND_IP, BIND_PORT))
    tprint(f"started TCP server on {BIND_IP}:{BIND_PORT}")

    sock.listen()
    sock.setblocking(False)

    while True:

        if VERBOSE:
            print_connections()

        # add new connection if there is one
        try:
            new_sock, addr = sock.accept()
            conn = Connection(new_sock, addr)
            connections.add(conn)
            tprint(f"{conn} connected.")
        except BlockingIOError:
            pass

        # try to call recv on all connections
        for conn in connections:
            try_recv(conn)

        # remove the connections that were disconnected
        clean_connections()

        # let cpu rest after all the hard work
        sleep(0.2)


def main() -> None:
    try:
        accept_loop()
    except KeyboardInterrupt:
        print()

    tprint(f"closing {len(connections)} connections")

    for conn in connections:
        conn.sock.close()

    tprint("exiting...")

if __name__ == "__main__":
    main()

