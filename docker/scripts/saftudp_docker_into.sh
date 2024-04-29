#!/usr/bin/env bash

docker exec \
    -u root \
    -it safe_udp \
    env LANG=C.UTF-8 \
    /bin/bash
