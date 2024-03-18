########################
# IPK24CHAT TCP server #
#   o k u r k a  1 2   #
########################

import socket
from time import sleep

BIND_IP = "0.0.0.0"
BIND_PORT = 4567
FAMILY = socket.AF_INET
SOCKTYPE = socket.SOCK_STREAM


class Message:
    pass


class Connection:
    def __init__(self, sock, addr) -> None:
        sock.setblocking(False)
        self.sock = sock
        self.addr = addr[0]
        self.port = addr[1]
        self.active = True
    def __repr__(self) -> str:
        return f"{self.addr}:{self.port}"
    def set_inactive(self) -> None:
        self.active = False

connections_to_remove = set()
connections = set()

def clean_connections():
    """
    remove inactive connections from global `connections`
    """
    global connections
    inactive_connections = {conn for conn in connections if not conn.active}
    connections = connections.difference(inactive_connections)

def print_connections() -> None:
    print(f"there are {len(connections)} clients connected: {connections}")

def try_recv(conn: Connection):
    sock = conn.sock
    try:
        data = sock.recv(65536)
        if (len(data) == 0):
            conn.set_inactive()
            return
        print(f"{len(data)} B from {conn}: {data}")

    except BlockingIOError:
        pass



def accept_loop() -> None:
    sock = socket.socket(FAMILY, SOCKTYPE)
    sock.bind((BIND_IP, BIND_PORT))
    print(f"started TCP server on {BIND_IP}:{BIND_PORT}")

    sock.listen()
    sock.setblocking(False)

    while True:

        # add new connection
        try:
            new_sock, addr = sock.accept()
            connections.add(Connection(new_sock, addr))
        except BlockingIOError:
            pass

        print_connections()


        for conn in connections:
            try_recv(conn)

        clean_connections()

        sleep(0.2)


def main() -> None:
    try:
        accept_loop()
    except KeyboardInterrupt:
        print()

    print("exiting...")

if __name__ == "__main__":
    main()

