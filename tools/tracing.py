#!/usr/bin/python
#coding: utf-8

import sys, os, re, subprocess, threading, datetime, time, shutil
import addr2line

def copy_file(src_file, dst_file):
   while True:
    data = src_file.read()
    if not data:
         break
    dst_file.write(data)

def start_process(lib_tracing_malloc_path, prog_args):
    args = 'LD_PRELOAD={0} {1}'.format(lib_tracing_malloc_path, ' '.join(map(str, prog_args)))
    popen = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    trd = threading.Thread(target=copy_file, args=(popen.stdout, sys.stdout))
    trd.daemon=True
    trd.start()
    trd = threading.Thread(target=copy_file, args=(popen.stderr, sys.stderr))
    trd.daemon=True
    trd.start()
    return popen

def collection_running(collection_interval_ms, pid, output_dir, last_collection_time_ms):
    try:
        snapshot_dst = ""
        maps_dst = ""

        if collection_interval_ms <= 0:
            return last_collection_time_ms

        now_ms = int(round(time.time() * 1000))
        next_collection_time_ms = last_collection_time_ms + collection_interval_ms
        if next_collection_time_ms > now_ms:
            return last_collection_time_ms

        now_str = datetime.datetime.now().strftime('%Y_%m_%d_%H_%M_%S_%f')
        snapshot_src = '{0}/{1}.{2}'.format('/tmp', 'tracing.malloc', pid) 
        snapshot_dst = '{0}/{1}.{2}.{3}'.format('/tmp', 'tracing.malloc', pid, now_str) 
        
        maps_src = '/proc/{0}/maps'.format(pid) 
        maps_dst = '/tmp/{0}.{1}.{2}.maps'.format('tracing.malloc', pid, now_str) 

        shutil.copyfile(snapshot_src, snapshot_dst)
        shutil.copyfile(maps_src, maps_dst)

        addr2line_output = '{0}/{1}.{2}.{3}'.format(output_dir, 'tracing.malloc', pid, now_str) 
        addr2line.analyze(snapshot_dst, maps_dst, addr2line_output)
        return now_ms
    finally:
        if snapshot_dst != "":
            os.remove(snapshot_dst)
        if maps_dst != "":
            os.remove(maps_dst)

def tracing(collection_interval_ms, output_dir, lib_tracing_malloc_path, prog_args):
    popen = start_process(lib_tracing_malloc_path, prog_args)
    pid = popen.pid
    
    last_collection_time_ms = 0
    while True:
        time.sleep(0.01)
        if popen.poll() != None:
            break
        last_collection_time_ms = collection_running(collection_interval_ms, pid, output_dir, last_collection_time_ms)

    snapshot = '{0}/{1}.{2}'.format('/tmp', 'tracing.malloc', pid) 
    maps = '/tmp/{0}.{1}.maps'.format('tracing.malloc', pid) 
    addr2line_output = '{0}/{1}.{2}'.format(output_dir, 'tracing.malloc', pid) 
    addr2line.analyze(snapshot, maps, addr2line_output)
    os.remove(snapshot)
    os.remove(maps)

def _show_usage():
    print(
        'usage: tracing [--collection_interval_ms=1000] prog-and-args.' + 
        '\n./tracing.py --collection_interval_ms=1000 ./alloc_comparison 1 1 1' +
        '\n./tracing.py ./alloc_comparison 1 1 1' 
    )

def _main():
    if len(sys.argv) == 1:
        return _show_usage()
    
    collection_interval_ms = 1000
    prog_args_start = 1
    collection_interval_ms_arg = sys.argv[1].split('=')
    if len(collection_interval_ms_arg) == 2 and collection_interval_ms_arg[0] == '--collection_interval_ms':
        collection_interval_ms = int(collection_interval_ms_arg[1])
        prog_args_start = 2
    prog_args = sys.argv[prog_args_start:]

    if len(prog_args) == 0:
        return _show_usage()

    lib_tracing_malloc_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'libtracing_malloc.so')
    tracing(collection_interval_ms, '.', lib_tracing_malloc_path, prog_args)

if __name__ == '__main__':
    _main()
