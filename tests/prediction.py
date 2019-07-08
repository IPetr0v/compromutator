#!/usr/bin/env python

import sys
from random import randint
from mininet.topolib import Topo
from mininet.link import Link, TCLink
from mininet.topo import LinearTopo

from performance import PredictionTest

if __name__ == '__main__':
    min_switches = int(sys.argv[1])
    max_switches = int(sys.argv[2])
    if len(sys.argv) > 3:
        delay = int(sys.argv[3])
    else:
        delay = 0

    if not delay:
        topologies = [LinearTopo(n, 1) for n in range(
            min_switches, max_switches + 1, 5)]
    else:
        topologies = [LinearTopo(n, 1, lopts={'cls': TCLink, 'delay': '%dus' % delay}) for n in range(
            min_switches, max_switches + 1, 5)]

    bandwidth_list = [int(b*1000000) for b in [0.1, 1, 10, 100, 1000]]

    print '--- Prediction Test ---'
    result_file = 'experiments/prediction_%d_%d_delay%d_id%d.csv' % (
        min_switches, max_switches, delay, randint(1000, 9999))
    test = PredictionTest(topologies, result_file=result_file,
                          flow_num=25, bandwidth_list=bandwidth_list,
                          delay=delay, run_times=1)
    delays = test.run()

    print 'Predictions'
    print delays
