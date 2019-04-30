#!/bin/sh
# 
# File:   build_run_example.sh
# Author: Alvaro Salazar <alvaro@denkitronik.com>
#
# Created on 29/04/2019, 11:55:52 PM
#
gcc examples/example.c -lwiringPi -lccs811 -o examples/example
sudo examples/example