#!/bin/bash
# sniffs on localhost and filters UDP packets on port 4567
sudo tcpdump -X -l -i lo port 4567