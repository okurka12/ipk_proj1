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

# server display name
SDNAME = "Server"

# how should the displayname of a client apper if he tried to
# rename as a server
ALT_SDNAME = "nice-try"

BIND_IP = "0.0.0.0"
BIND_PORT = 4567
FAMILY = socket.AF_INET
SOCKTYPE = socket.SOCK_STREAM

MTYPE_AUTH = "AUTH"
MTYPE_MSG = "MSG"
MTYPE_BYE = "BYE"
MTYPE_JOIN = "JOIN"

RE_USERNAME = r"((?:[A-Z]|[a-z]|[0-9]|-){1,20})"
RE_CHID = RE_USERNAME  # they're the same
RE_DISPLAYNAME = r"([!-~]{1,20})"
RE_SECRET = r"((?:[A-z]|[0-9]|-){1,128})"
RE_CONTENT = r"([ -~]{1,1400})"

AUTH_PATTERN = r"AUTH " + RE_USERNAME + r" AS " + RE_DISPLAYNAME  + \
r" USING " + RE_SECRET

MSG_PATTERN = r"MSG FROM " + RE_DISPLAYNAME + r" IS " + RE_CONTENT

JOIN_PATTERN = r"JOIN " + RE_CHID + r" AS " + RE_DISPLAYNAME

# timeout for the recv loop
# recommended: 0.2 if human uses client, else something lowe
# RL_TIMEO = 0.2
RL_TIMEO = 0.05


class Connection:
    """container for the socket and address"""
    def __init__(self, sock: socket.socket, addr: tuple[str, int]) -> None:
        sock.setblocking(False)
        self.sock = sock
        self.addr = addr[0]
        self.port = addr[1]
        self.active = True
        self.dname = ""  # displayname
        self.authenticated = False
    def __repr__(self) -> str:
        if len(self.dname) > 0:
            return f"{self.addr}:{self.port} (dname: {self.dname})"
        else:
            return f"{self.addr}:{self.port}"
    def set_inactive(self) -> None:
        if not self.active:
            return
        tprint(f"{self} disconnected.")
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError as e:
            tprint(f"OSError when calling shutdown: {e}")
        self.sock.close()
        self.active = False
        self.sock = None
    def send(self, data: bytes) -> None:
        """sends `data` to connection, but only if its active"""
        if not self.active:
            return
        if self.sock is not None:
            self.sock.sendall(data)
        else:
            tprint(f"Very weird, {self} socket is None")
    def send_err(self, text: str, dname: str=SDNAME) -> None:
        """like `Connection.send`, but for err messages"""
        text_shortened = text[:1300]
        text_data = f"ERR FROM {dname} IS {text_shortened}...\r\n"
        bin_data = text_data.encode("utf-8")
        self.send(bin_data)



class Message:
    """container for the parsed message + who it came from"""
    def __init__(self, data: bytes, conn: Connection) -> None:
        """
        parse message `data`, also save `conn` so it can be known where
        to send a reply to this message
        """

        self.type = "unknown"
        self.raw_data = data
        self.conn = conn

        # for situations like when someone sends two BYEs
        if not conn.active:
            return

        try:
            text = data.decode("ascii")
        except Exception as e:
            tprint(f"{conn} sent weird data: {data} it resulted in {e}")
            err_text = f"you sent weird bytes: {data.__repr__()}"
            self.conn.send_err(err_text)
            return

        # AUTH
        if text.lower().startswith("auth"):
            match_obj = re.fullmatch(AUTH_PATTERN, text, flags=re.IGNORECASE)
            if match_obj is None:
                err_text = f"couldn't parse as AUTH: {data.__repr__()}"
                tprint(err_text)
                self.conn.send_err(err_text)
                return
            self.type = MTYPE_AUTH
            self.username = match_obj[1]
            self.displayname = sanitize_dname(match_obj[2])
            self.conn.dname = sanitize_dname(match_obj[2])
            self.secret = match_obj[3]
            self.conn.authenticated = True

            # sanitize also raw data
            if match_obj[1].lower() == SDNAME.lower():
                new_raw_data = text.replace(match_obj[1], ALT_SDNAME)
                new_raw_data = new_raw_data.encode("utf-8")
                self.raw_data = new_raw_data

        # JOIN
        elif text.lower().startswith("join"):
            match_obj = re.fullmatch(JOIN_PATTERN, text, flags=re.IGNORECASE)
            if match_obj is None:
                err_text = f"couldn't parse as JOIN: {data.__repr__()}"
                tprint(err_text)
                self.conn.send_err(err_text)
                return
            self.type = MTYPE_JOIN
            self.displayname = sanitize_dname(match_obj[2])
            self.conn.dname = sanitize_dname(match_obj[2])
            # JOIN is not broadcasted, no need to sanitize the raw_content

        # MSG
        elif text.lower().startswith("msg"):
            match_obj = re.fullmatch(MSG_PATTERN, text, flags=re.IGNORECASE)
            if match_obj is None:
                err_text = f"couldn't parse as MSG: {data.__repr__()}"
                tprint(err_text)
                self.conn.send_err(err_text)
                return
            if match_obj[2].isspace():
                self.conn.send_err("Messages consisting of all spaces are "
                                   "valid regarding IPK24CHAT protocol, but "
                                   "I'm refusing them.")
                return
            self.type = MTYPE_MSG
            self.displayname = sanitize_dname(match_obj[1])
            self.conn.dname = sanitize_dname(match_obj[1])
            self.content = sanitize_content(match_obj[2])

            # sanitize also raw data
            if match_obj[1].lower() == SDNAME.lower():
                new_raw_data = text.replace(match_obj[1], ALT_SDNAME)
                new_raw_data = new_raw_data.encode("utf-8")
                self.raw_data = new_raw_data

        # ERR
        elif text.lower().startswith("err"):
            self.conn.send_err("I don't care about your ERR :) ")
            return

        # REPLY (reply from client???)
        elif text.lower().startswith("reply"):
            self.conn.send_err("tf, are you sending me REPLY as a client?")
            return

        # BYE
        elif text.lower().startswith("bye"):
            self.type = MTYPE_BYE
            self.conn.set_inactive()

        # message couldnt be parsed as anything
        if self.type == "unknown":
            err_text = f"couldn't parse message: {data.__repr__()}"
            tprint(err_text)
            self.conn.send_err(err_text)
            return



    def __repr__(self) -> str:
        pre = ""  # prefix
        suf = ""  # suffix
        delim = "\n"  # delimiter

        output = f"Type: {self.type}" + delim
        if self.username is not None:
            output += f"Username: {self.username}" + delim
        if self.displayname is not None:
            output += f"Display Name: {self.displayname}" + delim
        if self.secret is not None:
            output += f"Secret: {self.secret}" + delim
        if self.content is not None:
            output += f"Content: '{self.content}'"

        return output + suf

    def __getattr__(self, name: str):
        """eliminates exceptions by returning None"""
        return None


connections: Set[Connection] = set()

# broadcast queue
broad_q: list[Message] = []


def sanitize_dname(s: str) -> str:
    return s if s.lower() != SDNAME.lower() else ALT_SDNAME


def sanitize_content(s: str) -> str:
    return s.rstrip("\r\n").replace("\n", ".")


def print_time(lf=False):
    print(dt.datetime.now(), end="\n" if lf else ": ")


def tprint(*args, **kwargs):
    print_time()
    print(*args, **kwargs)

def vtprint(*args, **kwargs):
    """like `tprint` but only if `VERBOSE`"""
    if VERBOSE:
        tprint(*args, *kwargs)


def clean_connections():
    """
    remove inactive connections from global `connections`
    """
    global connections
    inactive_connections = {conn for conn in connections if not conn.active}
    connections = connections.difference(inactive_connections)


def print_connections() -> None:
    tprint(f"there are {len(connections)} clients connected: {connections}")


def add_disconnect_to_bq(conn: Connection) -> None:
    global broad_q
    msg = Message(b"BYE", conn)
    broad_q.append(msg)


def process_msg(msg: Message) -> None:
    """
    process the message, send an individual reply (REPLY) to `sock`
    todo: send MSGs to all?
    """

    # send REPLY to AUTH
    if msg.type == MTYPE_AUTH:
        vtprint(f"Sending REPLY with success={AUTH_SUCCES} to {msg.conn}")
        succ = "Successfully authenticated" if AUTH_SUCCES \
            else "Couldn't authenticate"
        reply_text = f"Hi, {msg.username}! {succ} you as {msg.displayname}"
        oknok = "OK" if AUTH_SUCCES else "NOK"
        whole_reply = f"REPLY {oknok} IS {reply_text}\r\n"
        msg.conn.send(whole_reply.encode("utf-8"))
        broad_q.append(msg)

    # send REPLY to JOIN
    if msg.type == MTYPE_JOIN:
        reply_text = f"Hi, {msg.conn.dname}! Successfully joined you " \
                     f"NOWHERE! This server has only one channel."
        whole_reply = f"REPLY OK IS {reply_text}\r\n"
        msg.conn.send(whole_reply.encode("utf-8"))

    # add MSG message to the broadcast queue
    if msg.type == MTYPE_MSG and "list-users" not in msg.content:
        broad_q.append(msg)

    # send list of users to the individual client
    if msg.type == MTYPE_MSG and "list-users" in msg.content:
        reply_text = f"MSG FROM {SDNAME} IS There are {len(connections)} " \
                     f"clients connected: "
        reply_text += ", ".join([conn.dname for conn in connections])
        reply_text += "\r\n"
        if msg.conn.sock is not None:
            msg.conn.send(reply_text.encode("utf-8"))
        else:
            tprint(f"weird, socket for {msg.conn} is none? (2)")

    # add BYE message to the broadcast queue
    # (this results in the "dname disconnected" message to clients)
    if msg.type == MTYPE_BYE:
        broad_q.append(msg)


def broadcast_messages() -> None:
    global connections
    global broad_q

    for msg in broad_q:
        for conn in connections:

            if not conn.authenticated:
                continue

            # dont send the message to who sent it
            if msg.type == MTYPE_MSG and conn != msg.conn:
                vtprint(f"sending MSG to {conn} (user msg)")
                data_crlf = bytearray(msg.raw_data)
                if not msg.raw_data.endswith(b"\r\n"):
                    data_crlf.extend(b"\r\n")
                conn.send(data_crlf)

            if msg.type == MTYPE_AUTH and conn != msg.conn:
                vtprint(f"sending MSG to {conn} (join broadcast)")
                text = f"MSG FROM {SDNAME} IS {msg.displayname} joined.\r\n"
                conn.send(text.encode("utf-8"))

            len_nonzero = len(msg.conn.dname) > 0
            if msg.type == MTYPE_BYE and conn != msg.conn and len_nonzero:
                vtprint(f"sending MSG to {conn} (disconnect broadcast)")
                text = f"MSG FROM {SDNAME} IS " \
                       f"{msg.conn.dname} disconnected.\r\n"
                conn.send(text.encode("utf-8"))


def broadcast_bye() -> None:
    global connections
    for conn in connections:
        conn.send(b"BYE\r\n")


def parse_many(data: bytes, conn: Connection) -> list[Message]:
    """
    split `data` by CRLF and return a list of the individual messages
    """
    output = []
    tprint(f"messages from {conn}")
    for ind_data in data.split(b"\r\n"):
        if len(ind_data) == 0:
            continue
        tprint(f"    raw {ind_data}")
        output.append(Message(ind_data, conn))
    return output


def try_recv(conn: Connection):
    """
    try to call recv on `conn` (non-blocking socket)
    if there is no data, do nothing
    """
    try:  # except BlockingIOError

        try:  # except ConnectionResetError
            data = conn.sock.recv(65536)
            if (len(data) == 0):
                conn.set_inactive()
                add_disconnect_to_bq(conn)
                return
        except ConnectionResetError:  # recv
            tprint(f"ConnectionResetError with {conn}")
            conn.set_inactive()
            add_disconnect_to_bq(conn)
            return
        except TimeoutError:  # recv
            tprint(f"TimeoutError with {conn}")
            conn.set_inactive()
            add_disconnect_to_bq(conn)
            return

        # parse the data
        vtprint(f"{len(data)} B from {conn}:")
        mesages = parse_many(data, conn)

        for msg in mesages:

            # send a reply
            process_msg(msg)

            # print the message
            if VERBOSE and PRINT_RAW:
                print(msg.raw_data)
            if VERBOSE:
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

        # broadcast what needs to be broadcasted
        broadcast_messages()
        broad_q.clear()

        # let cpu rest after all the hard work
        sleep(RL_TIMEO)


def main() -> None:
    try:
        accept_loop()
    except KeyboardInterrupt:
        # this is hard-coded here, run.sh depends on it!
        with open("server_tcp_end.log", "w", encoding="utf-8") as f:

            f.write("\n")
            f.write(str(dt.datetime.now()))
            f.write("\n")
            f.write(f"closing {len(connections)} connections:\n")
            f.write(str(connections))
            f.write("\n")


    clean_connections()
    broadcast_bye()

    # ideally, clients close the connection first upon receiving
    # BYE, so give them time to do that (not sure about this tho)
    sleep(0.1)

    for conn in connections:
        conn.set_inactive()  # this will close the socket

    # dont use this, avoid sigpipe when piping to `tee` :)
    # tprint("exiting...")

if __name__ == "__main__":
    main()

