#!/usr/bin/env python

import sys
from random import randint
from mininet.topo import LinearTopo

from performance import PredictionTest

if __name__ == '__main__':
    min_switches = int(sys.argv[1])
    max_switches = int(sys.argv[2])
    topologies = [LinearTopo(n, 1) for n in range(min_switches, max_switches + 1, 5)]
    bandwidth_list = [int(b*1000000) for b in [0.1, 1, 10, 100, 1000]]

    print '--- Prediction Test ---'
    result_file = 'experiments/prediction_%d_%d_id%d.csv' % (
        min_switches, max_switches, randint(1000, 9999))
    test = PredictionTest(topologies, result_file=result_file,
                          flow_num=50, bandwidth_list=bandwidth_list,
                          run_times=1)
    delays = test.run()

    print 'Predictions'
    print delays
