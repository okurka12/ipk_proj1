#!/bin/bash
# sniffs on localhost and filters UDP packets on port 4567
sudo tcpdump -X -l -i lo port 4567
# -X        ... header and data in hex + ascci
# -l        ... line buffered
# -i lo     ... interface `lo`
# port 4567 ... filter by port 4567 (src or dst) (this only filters AUTH
#               messages from client tho)
