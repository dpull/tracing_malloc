#!/usr/bin/python
#coding: utf-8

import sys
import os
import re
import subprocess  
import argparse

class Process:    
    def __init__(self, lib_tracing_malloc_path, prog_args):
        args = 'LD_PRELOAD={0} {1}'.format(lib_tracing_malloc_path, prog_args)
        self.popen = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

    def poll(self):
        return self.popen.poll()
    
    def kill(self):
        return self.popen.kill()

    

def main():
    parser = argparse.ArgumentParser(description='usage: tracing [options] "prog-and-args"')
    parser.add_argument('prog-and-args', metavar = 'N', type = str, nargs = '+', help='prog and args')
    parser.add_argument('--collection_interval_ms', default=1000, dest='collection_interval_ms', type = int, help='collection interval millisecond')

    args = parser.parse_args()
    print(args)
    print(args)

if __name__ == '__main__':
    main()
