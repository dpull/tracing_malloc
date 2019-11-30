#!/usr/bin/python
#coding: utf-8

import sys
import os
import re
import subprocess  
import argparse
import threading
import time
import addr2line

def os_copy_file(src_file, dst_file):
    retcode = os.system('cp {0} {1}'.format(src_file, dst_file))
    if retcode != 0:
        os.system('rm {0}'.format(dst_file))
        return False
    return True

def copy_file(src_file, dst_file):
   while True:
    data = src_file.read()
    if not data:
         break
    dst_file.write(data)

def start_process(lib_tracing_malloc_path, prog_args):
    args = 'LD_PRELOAD={0} {1}'.format(lib_tracing_malloc_path, prog_args)
    popen = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    trd = threading.Thread(target=copy_file, args=(popen.stdout, sys.stdout))
    trd.daemon=True
    trd.start()
    trd = threading.Thread(target=copy_file, args=(popen.stderr, sys.stderr))
    trd.daemon=True
    trd.start()
    return popen

def collection_running(collection_interval_ms, pid, output_dir, last_collection_time_ms):
    if collection_interval_ms <= 0:
        return last_collection_time_ms

    now = int(round(time.time() * 1000))
    next_collection_time_ms = last_collection_time_ms + collection_interval_ms
    if next_collection_time_ms > now:
        return last_collection_time_ms

    snapshot_src = '{0}/{1}.{2}'.format('/tmp', 'tracing.malloc', pid) 
    snapshot_dst = '{0}/{1}.{2}.{3}'.format('/tmp', 'tracing.malloc', pid, now) 
    
    maps_src = '/proc/{0}/maps'.format(pid) 
    maps_dst = '/tmp/{0}.{1}.{2}.maps'.format('tracing.malloc', pid, now) 

    if not os_copy_file(snapshot_src, snapshot_dst) or not  os_copy_file(maps_src, maps_dst):
        return last_collection_time_ms

    addr2line_output = '{0}/{1}.{2}.{3}'.format(output_dir, 'tracing.malloc', pid, now) 
    
    addr2line.analyze(snapshot_dst, maps_dst, addr2line_output)
    os.system('rm {0}'.format(snapshot_dst))
    os.system('rm {0}'.format(maps_dst))
    return now

def tracing(collection_interval_ms, output_dir, lib_tracing_malloc_path, prog_args):
    popen = start_process(lib_tracing_malloc_path, prog_args)
    pid = popen.pid
    
    last_collection_time_ms = 0
    while not popen.poll():
        time.sleep(0.01)
        last_collection_time_ms = collection_running(collection_interval_ms, pid, output_dir, last_collection_time_ms)

def _main():
    parser = argparse.ArgumentParser(description='usage: tracing --collection_interval_ms=1000 "prog-and-args"')
    parser.add_argument('prog-and-args', metavar = 'N', dest='prog_args', type = str, nargs = '+', help='prog and args')
    parser.add_argument('--collection_interval_ms', default=1000, dest='collection_interval_ms', type = int, help='collection interval millisecond')

    args = parser.parse_args()
    tracing(args.collection_interval_ms, "TODO", "TODO", args.prog_args)

if __name__ == '__main__':
    _main()
