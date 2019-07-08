#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


plt.rcParams.update({'font.size': 18})


def cut_by_item(dataframe, key, step):
    item_range = np.arange(0, dataframe[key].max(), step)
    grouped = dataframe.groupby(pd.cut(dataframe[key], item_range))
    keys = [key for key, _ in grouped]
    dataframes = [group for _, group in grouped]
    return keys, dataframes


def plot_delays_by_rules(dataframe):
    print 'Plotting by rules'
    bounds = [31000, 64000, 85000, 105000]
    #group_by_switches = dataframe.groupby(pd.cut(dataframe['network_size'], np.arange(0, 301, 100)))
    group_by_switches = dataframe.groupby(pd.cut(dataframe['network_size'], [0, 100, 200, 250, 300]))
    labels = [l for l, _ in group_by_switches]
    delays = [d for _, d in group_by_switches]

    #fig, axes = plt.subplots(2, 2)#, sharey=True)
    ax_indices = [(0, 0), (0, 1), (1, 0), (1, 1)]

    for label, delay, bound, idx in zip(labels, delays, bounds, ax_indices):
        rule_interval = 100
        grouped_by_rules = delay.groupby(pd.cut(
            delay['graph_size'],
            np.arange(0, bound, rule_interval))
            #np.arange(0, max(delay['graph_size']), rule_interval))
        )['duration']

        mean = grouped_by_rules.mean()
        #median = grouped_by_rules.median()
        lower = grouped_by_rules.quantile(0.05)
        upper = grouped_by_rules.quantile(0.95)

        #mean = mean.rolling(window=5, min_periods=1).mean()
        ##median = median.rolling(window=5, min_periods=1).mean()
        #lower = lower.rolling(window=5, min_periods=1).mean()
        #upper = upper.rolling(window=5, min_periods=1).mean()

        keys = [(k.left + k.right)/2.0 for k in mean.keys()]

        plt.figure(figsize=(8, 6))
        plt.ylim(0, 450)
        plt.plot(keys, mean, 'green')
        plt.fill_between(keys, lower, upper, color='green', alpha='0.15')
        plt.xlabel(u'Количество правил')#('Rule number')
        plt.ylabel(u'Задержка, мс')#('Delay, ms')
        #plt.title('Delay by rule number')
        plt.grid()
        plt.tight_layout()
        plt.savefig(data_dir + '/plot/' + 'delay_by_rules_%i_%i.pdf' % (label.left, label.right),
                    fmt='pdf')
        #plt.show()
        plt.gcf().clear()

        #ax = axes[idx[0]][idx[1]]
        #ax.plot(keys, mean, 'green')
        #ax.fill_between(keys, lower, upper, color='green', alpha='0.15')
        #ax.set_xlabel(u'Количество правил')#('Rule number')
        #ax.set_ylabel(u'Задержка, мс')#('Delay, ms')
        #ax.set_title(u'От %i до %i коммутаторов' % (label.left, label.right))#('Delay by rule number')
        #ax.grid()

    #fig.savefig(data_dir + '/plot/' + 'Delay by rule number', fmt='pdf')
    ##plt.show()
    #plt.gcf().clear()

    print 'Done'


def plot_delays_by_switches(dataframe):
    print 'Plotting by switches'
    switch_interval = 10
    switch_bound = 301

    dataframe = dataframe.sort_values(['graph_size'])
    dataframe['duration'] = dataframe['duration'].rolling(window=3, min_periods=1).mean()

    grouped_by_switches = dataframe.groupby(pd.cut(
        dataframe['network_size'],
        np.arange(0, switch_bound, switch_interval))
    )['duration']

    mean = grouped_by_switches.mean()
    #median = grouped_by_switches.median()
    #std = grouped_by_switches.std()
    lower = grouped_by_switches.quantile(0.05)
    upper = grouped_by_switches.quantile(0.95)

    keys = [(k.left + k.right)/2.0 for k in mean.keys()]

    mean = pd.Series(mean).reset_index(drop=True)
    #median = pd.Series(median).reset_index(drop=True)
    #std = pd.Series(std).reset_index(drop=True)
    lower = pd.Series(lower).reset_index(drop=True)
    upper = pd.Series(upper).reset_index(drop=True)

    #mean = mean.rolling(window=5, min_periods=1).mean()
    ##median = median.rolling(window=5, min_periods=1).mean()
    #lower = lower.rolling(window=5, min_periods=1).mean()
    #upper = upper.rolling(window=5, min_periods=1).mean()

    #plt.boxplot(data)
    #plt.plot(keys, median, yerr=std, color='blue')
    #plt.plot(keys, mean, 'ro')
    plt.figure(figsize=(14, 6))
    plt.errorbar(keys, mean, yerr=[lower, upper],
                 color='blue', fmt='o', markersize=9, capsize=6, elinewidth=3, capthick=2)
    #plt.fill_between(keys, lower, upper, color='blue', alpha='0.15')
    plt.xlabel(u'Количество коммутаторов')#('Network size')
    plt.ylabel(u'Задержка, мс')#('Delay, ms')
    #plt.title('Delay by network size')
    plt.grid()
    plt.tight_layout()
    plt.savefig(data_dir + '/plot/' + 'delay_by_switches.pdf', fmt='pdf')
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


def predictions_boxplot(dataframe, delay):
    #dataframe['packet_error'] = np.abs(dataframe['real_packets'] - dataframe['pred_packets'])
    #dataframe['byte_error'] = np.abs(dataframe['real_bytes'] - dataframe['pred_bytes'])

    #labels = list(reversed(['100 Kbps', '1 Mbps', '10 Mbps', '100 Mbps', '1 Gbps']))
    labels = list(reversed([u'100 Кбит/с', u'1 Мбит/с', u'10 Мбит/с', u'100 Мбит/с', u'1 Гбит/с']))
    bandwidths = reversed([int(b*1000000) for b in [0.1, 1, 10, 100, 1000]])
    packet_data = []
    byte_data = []
    for bandwidth in bandwidths:
        packet_data.append(dataframe[dataframe['bandwidth'] == bandwidth]['packet_error'])
        byte_data.append(dataframe[dataframe['bandwidth'] == bandwidth]['byte_error'])

    adjust_left = 0.21
    packet_xlabel = u'Ошибка предсказания (количество пакетов)'#'Packet error'
    byte_xlabel = u'Ошибка предсказания (количество байт)'#'Byte error'

    #b = plt.boxplot(packet_data, showfliers=False, vert=False)
    #print('Delay:', delay, '| Quantile:', [p.quantile(0.95) for p in packet_data])
    #print 'Delay:', delay, '| Quantile:', [item.get_xdata() for item in b['whiskers']]
    #return None  # DEBUG

    # Plot in normal scale
    plt.figure(figsize=(8, 6))
    plt.boxplot(packet_data, showfliers=False, vert=False)
    plt.yticks([1, 2, 3, 4, 5], labels)
    plt.xlabel(packet_xlabel)
    plt.tight_layout()
    plt.gcf().subplots_adjust(left=adjust_left)
    plt.savefig(data_dir + '/plot/' + 'packet_error_%d.pdf' % delay, fmt='pdf')
    #plt.show()
    plt.gcf().clear()

    # Plot in logarithmic scale
    ax = plt.gca()
    ax.set_xscale('log')
    plt.boxplot(packet_data, showfliers=False, vert=False)
    plt.yticks([1, 2, 3, 4, 5], labels)
    plt.xlabel(packet_xlabel)
    plt.tight_layout()
    plt.gcf().subplots_adjust(left=adjust_left)
    plt.savefig(data_dir + '/plot/' + 'packet_error_%d_log.pdf' % delay, fmt='pdf')
    #plt.show()
    plt.gcf().clear()

    # Plot in normal scale
    plt.boxplot(byte_data, showfliers=False, vert=False)
    plt.yticks([1, 2, 3, 4, 5], labels)
    plt.xlabel(byte_xlabel)
    plt.tight_layout()
    plt.gcf().subplots_adjust(left=adjust_left)
    plt.savefig(data_dir + '/plot/' + 'byte_error_%d.pdf' % delay, fmt='pdf')
    #plt.show()
    plt.gcf().clear()

    # Plot in logarithmic scale
    ax = plt.gca()
    ax.set_xscale('log')
    plt.boxplot(byte_data, showfliers=False, vert=False)
    plt.yticks([1, 2, 3, 4, 5], labels)
    plt.xlabel(byte_xlabel)
    plt.tight_layout()
    plt.gcf().subplots_adjust(left=adjust_left)
    plt.savefig(data_dir + '/plot/' + 'byte_error_%d_log.pdf' % delay, fmt='pdf')
    #plt.show()
    plt.gcf().clear()


def process_predictions(dataframe):
    dataframe = dataframe[dataframe['real_packets'] > 0]
    dataframe['packet_error'] = np.abs(dataframe['real_packets'] - dataframe['pred_packets'])
    dataframe['byte_error'] = np.abs(dataframe['real_bytes'] - dataframe['pred_bytes'])
    for delay in [0, 1, 33, 66, 333, 666, 2000]:
        predictions_boxplot(dataframe[dataframe['delay'] == delay], delay)


def load(files):
    dataframe_list = []
    for dataframe_file in files:
        df = pd.read_csv(dataframe_file, index_col=None, header=0)
        dataframe_list.append(df)
    return pd.concat(dataframe_list, ignore_index=True)


if __name__ == '__main__':
    data_dir = './experiments'

    delay_files = [data_dir + '/delay.csv']
    #delay_files = [data_dir + '/no_delay_prediction']
    prediction_files = [data_dir + '/prediction.csv']

    #print 'Loading delays'
    #delays = load(delay_files)
    #process_delays(delays)

    print 'Loading predictions'
    predictions = load(prediction_files)
    process_predictions(predictions)