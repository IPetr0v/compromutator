#!/bin/sh

cd ~/src/ryu/
#export PYTHONPATH=$PYTHONPATH:.

#ryu-manager ~/src/ryu/ryu/app/simple_switch_13.py ~/src/ryu/ryu/app/gui_topology/gui_topology.py \
#--observe-links --ofp-tcp-listen-port 6666

APPS='
ryu.app.gui_topology.gui_topology
ryu.app.simple_switch_13'
#ryu.app.simple_monitor_13 '
#APPS='
#ryu.app.sdnhub_apps.fileserver
#ryu.app.sdnhub_apps.host_tracker_rest
#ryu.app.rest_topology
#ryu.app.sdnhub_apps.stateless_lb_rest
#ryu.app.sdnhub_apps.tap_rest
#simple_switch_stp_13
#ryu.app.ofctl_rest'

./bin/ryu-manager ${APPS} --observe-links --ofp-tcp-listen-port 6633 --verbose
#./bin/ryu-manager --verbose