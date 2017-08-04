#!/usr/bin/env python

import sys

if len(sys.argv) != 2:
    sys.stderr.write('### error: expecting PBS output file\n')
    sys.exit(1)
file_name = sys.argv[1]
with open(file_name, 'r') as data_stream:
    thread_placement = dict()
    moving_threads = set()
    state = 'prolog'
    for line in data_stream:
        if state == 'prolog':
            if line.startswith('===='):
                state = 'parsing'
        elif state == 'parsing' and line.startswith('===='):
            break
        elif state == 'parsing' and line.startswith('succes'):
            break
        else:
            try:
                data = line.strip().split()
                thread = data[1]
                core = data[3]
                if thread in thread_placement:
                    if core != thread_placement[thread]:
                        moving_threads.add(thread)
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
    if moving_threads:
        print('Summary: {0:d}/{1:d} threads moved'.format(len(moving_threads),
                                                          len(thread_placement)))
        print('\t{0}'.format('\n\t'.join(moving_threads)))
