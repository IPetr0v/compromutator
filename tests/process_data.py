#!/usr/bin/env python

import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def cut_by_item(dataframe, key, step):
    item_range = np.arange(0, dataframe[key].max(), step)
    grouped = dataframe.groupby(pd.cut(dataframe[key], item_range))
    keys = [key for key, _ in grouped]
    dataframes = [group for _, group in grouped]
    return keys, dataframes


def plot_delays_by_rules(dataframe):
    print 'Plotting by rules'
    bounds = [32000, 64000, 100000]
    group_by_switches = dataframe.groupby(pd.cut(dataframe['network_size'], np.arange(0, 301, 100)))
    labels = [l for l, _ in group_by_switches]
    delays = [d for _, d in group_by_switches]

    #fig, axes = plt.subplots(1, 3)
    plt.ylim(0, 370)

    for label, delay, bound in zip(labels, delays, bounds):
        rule_interval = 100
        grouped_by_rules = delay.groupby(pd.cut(
            delay['graph_size'],
            np.arange(0, bound, rule_interval))
        )['duration']

        mean = grouped_by_rules.mean()
        #median = grouped_by_rules.median()
        lower = grouped_by_rules.quantile(0.10)
        upper = grouped_by_rules.quantile(0.90)

        mean = mean.rolling(window=5, min_periods=1).mean()
        #median = median.rolling(window=5, min_periods=1).mean()
        lower = lower.rolling(window=5, min_periods=1).mean()
        upper = upper.rolling(window=5, min_periods=1).mean()

        keys = [(k.left + k.right)/2.0 for k in mean.keys()]

        plt.plot(keys, mean, 'green')
        plt.fill_between(keys, lower, upper, color='green', alpha='0.15')
        plt.xlabel('Rule number')
        plt.ylabel('Delay, ms')
        #plt.title('Delay by rule number')
        plt.savefig(data_dir + '/plot/' + 'Delay by rule number %s.png' % str(label),
                    fmt='png')
        #plt.show()
        plt.gcf().clear()
    print 'Done'


def plot_delays_by_switches(dataframe):
    print 'Plotting by switches'
    switch_interval = 10
    switch_bound = 301

    grouped_by_switches = dataframe.groupby(pd.cut(
        dataframe['network_size'],
        np.arange(0, switch_bound, switch_interval))
    )['duration']

    mean = grouped_by_switches.mean()
    #median = grouped_by_switches.median()
    lower = grouped_by_switches.quantile(0.10)
    upper = grouped_by_switches.quantile(0.90)

    keys = [(k.left + k.right)/2.0 for k in mean.keys()]

    mean = pd.Series(mean).reset_index(drop=True)
    #median = pd.Series(median).reset_index(drop=True)
    lower = pd.Series(lower).reset_index(drop=True)
    upper = pd.Series(upper).reset_index(drop=True)

    #mean = mean.rolling(window=5, min_periods=1).mean()
    ##median = median.rolling(window=5, min_periods=1).mean()
    #lower = lower.rolling(window=5, min_periods=1).mean()
    #upper = upper.rolling(window=5, min_periods=1).mean()

    #plt.boxplot(data)
    #plt.plot(keys, median, yerr=std, color='blue')
    #plt.plot(keys, mean, 'ro')
    plt.errorbar(keys, mean, yerr=[lower, upper],
                 color='blue', fmt='o', capsize=3, elinewidth=1)
    #plt.fill_between(keys, lower, upper, color='blue', alpha='0.15')
    plt.xlabel('Network size')
    plt.ylabel('Delay, ms')
    plt.title('Delay by network size')
    plt.savefig(data_dir + '/plot/' + 'Delay by network size.png', fmt='png')
    #plt.show()
    plt.gcf().clear()

    # By mean table size
    #delays_table = delays
    print 'Done'


def process_delays(delays):
    delays = delays[delays['network_size'] % 5 == 0]
    delays = delays[delays['network_size'] <= 305]

    plot_delays_by_rules(delays)
    plot_delays_by_switches(delays)


def process_predictions(predictions):
    # Prediction
    #result['pred_packets'] = predicted['packet_count']
    #result['real_packets'] = real['packet_count']
    #result['pred_bytes'] = predicted['byte_count']
    #result['real_bytes'] = real['byte_count']
    #result['switch_num'] = testbed.switch_num()
    #result['rule_num'] = rule_num
    #result['flow_num'] = testbed.flow_num()
    #result['load'] = testbed.network_load()

    predictions['packet_error'] = np.square(predictions['real_packets'] -
                                            predictions['pred_packets'])
    predictions['byte_error'] = np.square(predictions['real_bytes'] -
                                          predictions['pred_bytes'])

    prediction_by_bandwidth = predictions.groupby(
        ['bandwidth'])
    prediction_by_switch_num = predictions.groupby(
        ['switch_num'])
    dataframes = [g for g in prediction_by_bandwidth]

    keys = [k for k, _ in prediction_by_switch_num]
    labels = [l for l, _ in dataframes]
    errors = [df.groupby(['switch_num'])['packet_error'].mean() for _, df in dataframes]
    #for key, df in dataframes:
    #    keys.append(key)
    #    prediction_by_switch_num = df.groupby(
    #        ['switch_num'])['packet_error'].mean()
    #    predictions_by_switch_num.append(prediction_by_switch_num)

    data = []

    for label, error in zip(labels, errors):
        print 'keys', len(keys), keys
        print 'error', len(error.keys()), error.keys()
        for k, e in zip(keys, error.keys()):
            print k, e
        plt.plot(keys, error, label=label)
    plt.legend()
    #plt.boxplot(data)
    #plt.xticks([1, 2, 3, 4, 5],#[int(b*1000000) for b in [0.1, 1, 10, 100, 1000]],
    #           ['100 Kbps', '1 Mbps', '10 Mbps', '100 Mbps', '1 Gbps'])
    #plt.savefig(data_dir + '/plot/' + 'Delay by network size.png', fmt='png')
    plt.show()
    plt.gcf().clear()


def load(files):
    dataframe_list = []
    for dataframe_file in files:
        df = pd.read_csv(dataframe_file, index_col=None, header=0)
        dataframe_list.append(df)
    return pd.concat(dataframe_list, ignore_index=True)


if __name__ == '__main__':
    data_dir = './experiments'

    delay_files = [data_dir + '/delay.csv']
    #prediction_files = [data_dir + '/prediction.csv']

    print 'Loading delays'
    delays = load(delay_files)
    process_delays(delays)

    print 'Loading predictions'
    predictions = load(prediction_files)
    process_predictions(predictions)