#!/bin/bash
export DISPLAY=:0.0
export LD_LIBRARY_PATH="$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib"
cd /home/scotty/jn-engine
echo "Starting jnengine..." > /tmp/jn_run.log
./jnengine >> /tmp/jn_run.log 2>&1
echo "Exit code: $?" >> /tmp/jn_run.log
