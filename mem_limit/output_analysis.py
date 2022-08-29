#!/usr/bin/env python
# coding: utf-8

from argparse import ArgumentParser
from collections import defaultdict
from datetime import datetime
import pandas as pd
import re


def parse_log_file(file_name):
    data = defaultdict(list)
    date_pattern = r'(?P<time>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+)'
    placement_pattern = r': rank (?P<id>\S+) on (?P<core>\S+): '
    msg_pattern = r'(?P<msg>.+)'
    log_regex = re.compile(date_pattern + placement_pattern + msg_pattern)
    with open(file_name) as file:
        for line in file:
            line = line.strip()
            if 'running' in line or 'success' in line:
                continue
            else:
                matches = log_regex.search(line)
                if matches is not None:
                    data['timestamp'].append(datetime.strptime(matches.group('time'), '%Y-%m-%d %H:%M:%S.%f'))
                    rank, thread_id = matches.group('id').split('#')
                    data['rank'].append(int(rank))
                    data['thread_id'].append(int(thread_id))
                    core, host = matches.group('core').split('@')
                    data['core'].append(int(core))
                    data['host'].append(host)
                    data['event_type'].append('allocate' if 'alloc' in matches.group('msg') else 'fill')
        return pd.DataFrame(data)


if __name__ == '__main__':

    arg_parser = ArgumentParser(description='parse mem_limit output to verify binding')
    arg_parser.add_argument('file_name', help='name of mem_limit output file')
    options = arg_parser.parse_args()
    log_data = parse_log_file(options.file_name)
    bindings = log_data[['rank', 'thread_id', 'host', 'core']] \
                    .drop_duplicates() \
                    .groupby(['rank', 'thread_id', 'host']) \
                    .aggregate(lambda tdf: tdf.unique().tolist())
    print(bindings)
