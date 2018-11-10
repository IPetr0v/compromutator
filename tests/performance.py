#!/usr/bin/env python

import re
import pandas as pd
from random import shuffle
from retry import retry

from sys import argv
import matplotlib.pyplot as plt
from time import time, sleep
from mininet.topo import LinearTopo

from stand import TestStand


class MaxPingDelay:
    def __init__(self, size=10, run_num=1, ideal=False):
        self.size = size
        self.run_num = run_num
        self.ideal = ideal

    @retry(exceptions=RuntimeError, tries=5)
    def run(self):
        results = []
        stand = None
        try:
            for _ in range(self.run_num):
                # Create test stand
                if self.ideal:
                    stand = self.get_ideal_stand()
                else:
                    stand = self.get_stand()
                stand.start()
                sleep(3)

                # Create host ping sequence
                ping_pairs = []
                hosts = stand.network.hosts
                for i in range(len(hosts)):
                    for j in range(i+1, len(hosts)):
                        ping_pairs.append((i, j))
                shuffle(ping_pairs)

                for pair in ping_pairs:
                    # Run ping test
                    result = self.ping(
                        stand, src=hosts[pair[0]], dst=hosts[pair[1]])
                    results.append(result)

                # Stop test stand
                stand.stop()
        except KeyboardInterrupt:
            stand.stop()

        return pd.DataFrame(results)

    def ping(self, stand, src, dst):
        result = dict()
        # TODO: remove sleep and fix rule installation
        sleep(1)
        result['switch_num'] = stand.switch_num()
        if self.ideal:
            result['rule_num'] = stand.rule_num(count_zero=True)
        else:
            result['rule_num'] = stand.rule_num(count_zero=False)

        print 'switch_num', result['switch_num']
        print 'rule_num', result['rule_num']

        # Get first ping since only it influences rule installation
        output = stand.network.pingFull(hosts=[src, dst], timeout=0.5)[0]

        #result['src'] = output[0]
        #result['dst'] = output[1]
        ping_output = output[2]

        #result['sent'] = ping_output[0]
        #result['received'] = ping_output[1]

        #result['min_rtt'] = ping_output[2]
        #result['avg_rtt'] = ping_output[3]
        #result['max_rtt'] = ping_output[4]
        #result['dev_rtt'] = ping_output[5]

        # Get max RTT
        result['RTT'] = ping_output[4]

        if result['RTT'] == 0.0:
            raise RuntimeError('Ping failed on %d switches' % self.size)

        return result

    def get_stand(self):
        TestStand.clear()
        stand = TestStand(
            port=6653,
            controller_port=6633,
            topo=LinearTopo(self.size, 1),
            run_compromutator=True,
            log_file='log.txt')
        return stand

    def get_ideal_stand(self):
        TestStand.clear()
        stand = TestStand(
            port=6653,
            controller_port=6653,
            topo=LinearTopo(self.size, 1),
            run_compromutator=False,
            log_file='log.txt')
        return stand

class PerformanceTest:
    def __init__(self, test_class, switch_list, run_num):
        self.test_class = test_class
        self.switch_list = switch_list
        self.run_num = run_num

    def run(self):
        real_results = []
        ideal_results = []
        for switch_num in self.switch_list:
            real_test = self.test_class(switch_num, self.run_num)
            ideal_test = self.test_class(switch_num, self.run_num, ideal=True)

            real_results.append(real_test.run())
            ideal_results.append(ideal_test.run())

        return pd.concat(real_results).reset_index(drop=True),\
               pd.concat(ideal_results).reset_index(drop=True)


if __name__ == '__main__':
    test = PerformanceTest(MaxPingDelay, switch_list=range(2, 103, 5), run_num=3)
    real_results, ideal_results = test.run()
    print '--- real_results'
    print real_results
    print '--- ideal_results'
    print ideal_results

    real_results.to_csv('real_rtt.csv', index=False)
    ideal_results.to_csv('ideal_rtt.csv', index=False)

    #print '\n====== Results real:ideal ======'
    #for result in results:
    #    print('%d: %.2f|%.2f' % (
    #        result['switch_num'],
    #        result['real'],
    #        result['ideal']))

    #switch_num_list = [r['switch_num'] for r in results]
    #real_results = [r['real'] for r in results]
    #ideal_results = [r['ideal'] for r in results]

    #plt.plot(switch_num_list, real_results, 'r')
    #plt.plot(switch_num_list, ideal_results, 'b')
    ##plt.axis([0, 6, 0, 20])
    #plt.show()