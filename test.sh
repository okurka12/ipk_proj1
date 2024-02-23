RED='\033[0;31m'
COLOR_RESET='\033[0m'

echo_ok () {
    echo $@
}

echo_err () {
    echo -e "$RED$@$RESET"
}

test_arg () {
    ./test_argparse.bin $@ > /dev/null 2> /dev/null
    RESULT=$?
    if [ $RESULT == $EXPECTED ]; then
        echo_ok "OK: '$@'"
    else
        echo_err "ERR: '$@' (expected $EXPECTED got $RESULT)"
    fi
}

# -t tcp|udp -s ADDRESS [-p PORT] [-d TIMEOUT] [-r RETRIES] [-h]
make test_argparse.bin
EXPECTED=0
test_arg -s localhost -t udp
test_arg -s vitapavlik.cz -t tcp
test_arg -s vitapavlik.cz -t tcp -p
test_arg -t udp -s 127.0.0.1 -p 123 -d 50 -r 5
test_arg -t udp -s 127.0.0.1 -p 123 -r 5 -d 50
test_arg -t udp -s 127.0.0.1 -d 50 -p 123 -r 5
test_arg -t udp -s 127.0.0.1 -d 50 -r 5 -p 123
test_arg -t udp -s 127.0.0.1 -r 5 -p 123 -d 50
test_arg -t udp -s 127.0.0.1 -r 5 -d 50 -p 123
test_arg -t udp -p 123 -s 127.0.0.1 -d 50 -r 5
test_arg -t udp -p 123 -s 127.0.0.1 -r 5 -d 50
test_arg -t udp -p 123 -d 50 -s 127.0.0.1 -r 5
test_arg -t udp -p 123 -d 50 -r 5 -s 127.0.0.1
test_arg -t udp -p 123 -r 5 -s 127.0.0.1 -d 50
test_arg -t udp -p 123 -r 5 -d 50 -s 127.0.0.1
test_arg -t udp -d 50 -s 127.0.0.1 -p 123 -r 5
test_arg -t udp -d 50 -s 127.0.0.1 -r 5 -p 123
test_arg -t udp -d 50 -p 123 -s 127.0.0.1 -r 5
test_arg -t udp -d 50 -p 123 -r 5 -s 127.0.0.1
test_arg -t udp -d 50 -r 5 -s 127.0.0.1 -p 123
test_arg -t udp -d 50 -r 5 -p 123 -s 127.0.0.1
test_arg -t udp -r 5 -s 127.0.0.1 -p 123 -d 50
test_arg -t udp -r 5 -s 127.0.0.1 -d 50 -p 123
test_arg -t udp -r 5 -p 123 -s 127.0.0.1 -d 50
test_arg -t udp -r 5 -p 123 -d 50 -s 127.0.0.1
test_arg -t udp -r 5 -d 50 -s 127.0.0.1 -p 123
test_arg -t udp -r 5 -d 50 -p 123 -s 127.0.0.1
test_arg -s 127.0.0.1 -t udp -p 123 -d 50 -r 5
test_arg -s 127.0.0.1 -t udp -p 123 -r 5 -d 50
test_arg -s 127.0.0.1 -t udp -d 50 -p 123 -r 5
test_arg -s 127.0.0.1 -t udp -d 50 -r 5 -p 123
test_arg -s 127.0.0.1 -t udp -r 5 -p 123 -d 50
test_arg -s 127.0.0.1 -t udp -r 5 -d 50 -p 123
test_arg -s 127.0.0.1 -p 123 -t udp -d 50 -r 5
test_arg -s 127.0.0.1 -p 123 -t udp -r 5 -d 50
test_arg -s 127.0.0.1 -p 123 -d 50 -t udp -r 5
test_arg -s 127.0.0.1 -p 123 -d 50 -r 5 -t udp
test_arg -s 127.0.0.1 -p 123 -r 5 -t udp -d 50
test_arg -s 127.0.0.1 -p 123 -r 5 -d 50 -t udp
test_arg -s 127.0.0.1 -d 50 -t udp -p 123 -r 5
test_arg -s 127.0.0.1 -d 50 -t udp -r 5 -p 123
test_arg -s 127.0.0.1 -d 50 -p 123 -t udp -r 5
test_arg -s 127.0.0.1 -d 50 -p 123 -r 5 -t udp
test_arg -s 127.0.0.1 -d 50 -r 5 -t udp -p 123
test_arg -s 127.0.0.1 -d 50 -r 5 -p 123 -t udp
test_arg -s 127.0.0.1 -r 5 -t udp -p 123 -d 50
test_arg -s 127.0.0.1 -r 5 -t udp -d 50 -p 123
test_arg -s 127.0.0.1 -r 5 -p 123 -t udp -d 50
test_arg -s 127.0.0.1 -r 5 -p 123 -d 50 -t udp
test_arg -s 127.0.0.1 -r 5 -d 50 -t udp -p 123
test_arg -s 127.0.0.1 -r 5 -d 50 -p 123 -t udp
test_arg -p 123 -t udp -s 127.0.0.1 -d 50 -r 5
test_arg -p 123 -t udp -s 127.0.0.1 -r 5 -d 50
test_arg -p 123 -t udp -d 50 -s 127.0.0.1 -r 5
test_arg -p 123 -t udp -d 50 -r 5 -s 127.0.0.1
test_arg -p 123 -t udp -r 5 -s 127.0.0.1 -d 50
test_arg -p 123 -t udp -r 5 -d 50 -s 127.0.0.1
test_arg -p 123 -s 127.0.0.1 -t udp -d 50 -r 5
test_arg -p 123 -s 127.0.0.1 -t udp -r 5 -d 50
test_arg -p 123 -s 127.0.0.1 -d 50 -t udp -r 5
test_arg -p 123 -s 127.0.0.1 -d 50 -r 5 -t udp
test_arg -p 123 -s 127.0.0.1 -r 5 -t udp -d 50
test_arg -p 123 -s 127.0.0.1 -r 5 -d 50 -t udp
test_arg -p 123 -d 50 -t udp -s 127.0.0.1 -r 5
test_arg -p 123 -d 50 -t udp -r 5 -s 127.0.0.1
test_arg -p 123 -d 50 -s 127.0.0.1 -t udp -r 5
test_arg -p 123 -d 50 -s 127.0.0.1 -r 5 -t udp
test_arg -p 123 -d 50 -r 5 -t udp -s 127.0.0.1
test_arg -p 123 -d 50 -r 5 -s 127.0.0.1 -t udp
test_arg -p 123 -r 5 -t udp -s 127.0.0.1 -d 50
test_arg -p 123 -r 5 -t udp -d 50 -s 127.0.0.1
test_arg -p 123 -r 5 -s 127.0.0.1 -t udp -d 50
test_arg -p 123 -r 5 -s 127.0.0.1 -d 50 -t udp
test_arg -p 123 -r 5 -d 50 -t udp -s 127.0.0.1
test_arg -p 123 -r 5 -d 50 -s 127.0.0.1 -t udp
test_arg -d 50 -t udp -s 127.0.0.1 -p 123 -r 5
test_arg -d 50 -t udp -s 127.0.0.1 -r 5 -p 123
test_arg -d 50 -t udp -p 123 -s 127.0.0.1 -r 5
test_arg -d 50 -t udp -p 123 -r 5 -s 127.0.0.1
test_arg -d 50 -t udp -r 5 -s 127.0.0.1 -p 123
test_arg -d 50 -t udp -r 5 -p 123 -s 127.0.0.1
test_arg -d 50 -s 127.0.0.1 -t udp -p 123 -r 5
test_arg -d 50 -s 127.0.0.1 -t udp -r 5 -p 123
test_arg -d 50 -s 127.0.0.1 -p 123 -t udp -r 5
test_arg -d 50 -s 127.0.0.1 -p 123 -r 5 -t udp
test_arg -d 50 -s 127.0.0.1 -r 5 -t udp -p 123
test_arg -d 50 -s 127.0.0.1 -r 5 -p 123 -t udp
test_arg -d 50 -p 123 -t udp -s 127.0.0.1 -r 5
test_arg -d 50 -p 123 -t udp -r 5 -s 127.0.0.1
test_arg -d 50 -p 123 -s 127.0.0.1 -t udp -r 5
test_arg -d 50 -p 123 -s 127.0.0.1 -r 5 -t udp
test_arg -d 50 -p 123 -r 5 -t udp -s 127.0.0.1
test_arg -d 50 -p 123 -r 5 -s 127.0.0.1 -t udp
test_arg -d 50 -r 5 -t udp -s 127.0.0.1 -p 123
test_arg -d 50 -r 5 -t udp -p 123 -s 127.0.0.1
test_arg -d 50 -r 5 -s 127.0.0.1 -t udp -p 123
test_arg -d 50 -r 5 -s 127.0.0.1 -p 123 -t udp
test_arg -d 50 -r 5 -p 123 -t udp -s 127.0.0.1
test_arg -d 50 -r 5 -p 123 -s 127.0.0.1 -t udp
test_arg -r 5 -t udp -s 127.0.0.1 -p 123 -d 50
test_arg -r 5 -t udp -s 127.0.0.1 -d 50 -p 123
test_arg -r 5 -t udp -p 123 -s 127.0.0.1 -d 50
test_arg -r 5 -t udp -p 123 -d 50 -s 127.0.0.1
test_arg -r 5 -t udp -d 50 -s 127.0.0.1 -p 123
test_arg -r 5 -t udp -d 50 -p 123 -s 127.0.0.1
test_arg -r 5 -s 127.0.0.1 -t udp -p 123 -d 50
test_arg -r 5 -s 127.0.0.1 -t udp -d 50 -p 123
test_arg -r 5 -s 127.0.0.1 -p 123 -t udp -d 50
test_arg -r 5 -s 127.0.0.1 -p 123 -d 50 -t udp
test_arg -r 5 -s 127.0.0.1 -d 50 -t udp -p 123
test_arg -r 5 -s 127.0.0.1 -d 50 -p 123 -t udp
test_arg -r 5 -p 123 -t udp -s 127.0.0.1 -d 50
test_arg -r 5 -p 123 -t udp -d 50 -s 127.0.0.1
test_arg -r 5 -p 123 -s 127.0.0.1 -t udp -d 50
test_arg -r 5 -p 123 -s 127.0.0.1 -d 50 -t udp
test_arg -r 5 -p 123 -d 50 -t udp -s 127.0.0.1
test_arg -r 5 -p 123 -d 50 -s 127.0.0.1 -t udp
test_arg -r 5 -d 50 -t udp -s 127.0.0.1 -p 123
test_arg -r 5 -d 50 -t udp -p 123 -s 127.0.0.1
test_arg -r 5 -d 50 -s 127.0.0.1 -t udp -p 123
test_arg -r 5 -d 50 -s 127.0.0.1 -p 123 -t udp
test_arg -r 5 -d 50 -p 123 -t udp -s 127.0.0.1
test_arg -r 5 -d 50 -p 123 -s 127.0.0.1 -t udp
