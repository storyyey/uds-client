#!/bin/sh

sudo pkill -f diag_master
rm /tmp/om_process.lock
chmod +x diag_master
#./diag_master -m -s60
#./diag_master -m -s60 -d
./diag_master -m -s60 -D./ydm_log/

