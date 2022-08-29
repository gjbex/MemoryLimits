#!/usr/bin/env python

from argparse import ArgumentParser
from collections import defaultdict
import sys


def process_key(pid):
    rank, thread = pid.split('#')
    return 10000*int(rank) + int(thread)

if __name__ == '__main__':
    arg_parser = ArgumentParser(description='parse memlimit output to '
                                            'detect threads that '
                                            'wandered around')
    arg_parser.add_argument('file', help='output file to parse')
    arg_parser.add_argument('--pbs', action='store_true',
                            help='output is part of PBS job output')
    arg_parser.add_argument('--verbose', action='store_true',
                            help='generate verbose output')
    options = arg_parser.parse_args()
    with open(options.file, 'r') as data_stream:
        thread_placement = dict()
        thread_locations = defaultdict(set)
        moving_threads = set()
        nr_moves = 0
        if options.pbs:
            state = 'prolog'
        else:
            state = 'parsing'
        for line in data_stream:
            if state == 'prolog':
                if line.startswith('===='):
                    state = 'parsing'
            elif state == 'parsing' and line.startswith('running'):
                continue
            elif state == 'parsing' and line.startswith('===='):
                break
            elif state == 'parsing' and 'success' in line:
                break
            elif state == 'parsing' and 'running' in line:
                continue
            else:
                try:
                    data = line.strip().split()
                    thread = data[3]
                    core = data[5].strip(':')
                    thread_locations[thread].add(core)
                    if thread in thread_placement:
                        if core != thread_placement[thread]:
                            moving_threads.add(thread)
                            nr_moves += 1
                            if options.verbose:
                                msg = 'thread {0} moved from {1} to {2}'
                                print(msg.format(thread,
                                                 thread_placement[thread],
                                                 core))
                    else:
                        thread_placement[thread] = core
                except Exception as e:
                    msg = "### error: {0} on line\n\t'{1}'"
                    sys.stderr.write(msg.format(str(e), line))
                    sys.exit(2)
        if moving_threads or options.verbose:
            msg = 'Summary: {0:d}/{1:d} threads moved, {2:d} total moves'
            print(msg.format(len(moving_threads), len(thread_placement),
                             nr_moves))
            if options.verbose:
                print('placement:')
                for thread, locations in thread_locations.items():
                    print('{thread:10s}: {core:s}'.format(thread=thread, core=','.join(locations)))
