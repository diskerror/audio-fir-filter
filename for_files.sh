#!/bin/bash
#
# Brief description of your script
# Copyright 2025 reid

for file in "$@" ; do
  (./lowcut -f 30 -s 20 "$file" >/dev/null ) &
  sleep 1
done
