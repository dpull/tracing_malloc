#!/usr/bin/python
#coding: utf-8

import sys
import re
import subprocess  
import argparse

class Process:    
    def __init__(self, preload, prog_args):
        pass

def main():
    parser = argparse.ArgumentParser(description='usage: tracing [options] "prog-and-args"')
    parser.add_argument('prog-and-args', metavar = 'N', type = str, nargs = '+', help='prog and args')
    parser.add_argument('--collection_interval', default=5, dest='collection_interval', type = int, help='collection_interval')
    args = parser.parse_args()

if __name__ == '__main__':
    main()
