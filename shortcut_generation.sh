#!/bin/sh
cd diag_master
make
cd -
cd diag_master_api && make all && cd -
cd demo
make SOURCE_FILE=01_demo
make SOURCE_FILE=02_demo
make SOURCE_FILE=03_demo
make SOURCE_FILE=04_demo
make SOURCE_FILE=05_demo
make SOURCE_FILE=06_demo
make SOURCE_FILE=07_demo
make SOURCE_FILE=08_demo
make SOURCE_FILE=09_demo
make SOURCE_FILE=10_demo
make SOURCE_FILE=11_demo
make SOURCE_FILE=12_demo
make SOURCE_FILE=13_demo
make SOURCE_FILE=14_demo
make SOURCE_FILE=15_demo
make SOURCE_FILE=16_demo
make SOURCE_FILE=17_demo
make SOURCE_FILE=18_demo
make SOURCE_FILE=19_demo
make SOURCE_FILE=20_demo
make SOURCE_FILE=api_test
cd -

