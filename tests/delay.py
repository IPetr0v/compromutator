#!/usr/bin/env python

import sys
from random import randint
from mininet.topo import LinearTopo

from performance import DelayTest

if __name__ == '__main__':
    min_switches = int(sys.argv[1])
    max_switches = int(sys.argv[2])
    topologies = [LinearTopo(n, 1) for n in range(min_switches, max_switches + 1, 5)]

    print '--- Delay Test ---'
    result_file = 'experiments/delay_%d_%d_id%d.csv' % (
        min_switches, max_switches, randint(1000, 9999))
    test = DelayTest(topologies, result_file=result_file, run_times=2)
    delays = test.run()

    print 'Delays'
    print delays
