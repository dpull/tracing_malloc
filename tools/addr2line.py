#!/usr/bin/python
#coding: utf-8

import sys
import re
import subprocess  
import argparse
import struct

class Addr2Line:
    def __init__(self):
        self.__fbase_2_fname = {}
        self.__fbase_2_addrset = {}
        self.__is_shared_object_cache = {}
        self.__address_2_line = {}

    def add(self, address, fbase, fname):
        self.__fbase_2_fname[fbase] = fname
        self.__fbase_2_addrset.setdefault(fbase, set())
        self.__fbase_2_addrset[fbase].add(address)

    def execute(self):
        for fbase, addrset in self.__fbase_2_addrset.items():
            fname = self.__fbase_2_fname[fbase]
            self.__popen(addrset, fbase, fname)

    def get_from_cache(self, address):
        return self.__address_2_line.get(address)

    def __popen(self, addrset, fbase, fname):
        is_so = self.__is_shared_object(fname)

        addrlist = list(addrset)
        if is_so:
            for i in range(len(addrlist)):
                addrlist[i] = addrlist[i] - fbase
        
        pcmd = 'addr2line -Cfse {0}'.format(fname)
        for addr in addrlist:
            pcmd = '{0}  0x{1:x}'.format(pcmd, addr)
        
        p = subprocess.Popen(pcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for addr in addrlist:
            addr_raw = addr if not is_so else addr + fbase
            function = self.__to_string(p.stdout.readline()).rstrip('\n')
            file = self.__to_string(p.stdout.readline()).rstrip('\n')
            line = "{0}\t{1}\t{2}".format(function, file, fname)
            self.__address_2_line[addr_raw] = line

    def __is_shared_object(self, file_path):
        if file_path not in self.__is_shared_object_cache: 
            with open(file_path, 'rb') as elf:
                elf.seek(16) # ei_ident
                ei_type = struct.unpack('H', elf.read(2))[0]
                self.__is_shared_object_cache[file_path] = ei_type == 3
        return self.__is_shared_object_cache[file_path]

    def __to_string(self, str_or_bytes): # compatible with Python2 and Python3. 
        if type(str_or_bytes) != str:
            str_or_bytes = str_or_bytes.decode('utf-8')
        return str_or_bytes        

def load_bin_file(file_path):
    with open(file_path, 'rb') as file:
        fmt = '=qqqq28q' # ptr, time, size, reserve, stack
        size = struct.calcsize(fmt)
        data = []
        while True:
            line = file.read(size)
            if not line:
                break
            assert(len(line) == size)
            unpack_data = struct.unpack(fmt, line)
            if unpack_data[0] != 0 and unpack_data[0] != -1:
                for index, value in enumerate(reversed(unpack_data)):
                    if value != 0:
                        index = len(unpack_data) - index
                        break
                line_data = {'ptr' : unpack_data[0], 'time' : unpack_data[1], 'size' : unpack_data[2], 'stack' : unpack_data[4:index]}
                data.append(line_data)
        return data

def save_file(file_path, data):
    with open(file_path, 'w') as file:
        for item in data:
            file.write('time:{0}\tsize:{1}\tptr:0x{2:x}\n'.format(item['time'], item['size'], item['ptr']))
            for frame in item['stack_line']:
                file.write('{0}\n'.format(frame))
            file.write('========\n\n')

def load_data(file_path, maps, output_path):
    data = load_bin_file(file_path)
    data.sort(key = lambda x : x['time'])

    addr2line = Addr2Line()

    for item in data:
        for address in item['stack']:
            line = '0x{:x}'.format(address)
            moudle = list(filter(lambda x : address >= x['start'] and address < x['end'], maps))
            if moudle:
                addr2line.add(address, moudle[0]['start'], moudle[0]['pathname'])

    addr2line.execute()

    for item in data:
        stack_line = []
        for address in item['stack']:
            line = addr2line.get_from_cache(address)
            if not line:
                line = '0x{:x}'.format(address)
            stack_line.append(line)
        item['stack_line'] = stack_line

    save_file(output_path, data)

def _extract_maps(line):
    try:
        maps = re.match(r'([0-9A-Fa-f]+)-([0-9A-Fa-f]+)\s+([-r][-w][-x][sp])\s+([0-9A-Fa-f]+)\s+([:0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+(.*)$', line)
        start = int(maps.group(1), 16)
        end = int(maps.group(2), 16)
        perms = maps.group(3)
        offset = int(maps.group(4), 16)
        dev = maps.group(5)
        inode = maps.group(6)
        pathname = maps.group(7)
    except:
        print('_extract_maps failed:', line)
        raise
    return { 'start': start, 'end': end, 'perms': perms, 'offset': offset, 'dev': dev, 'inode': inode, 'pathname': pathname }

def load_maps(file_path):
    with open(file_path, 'r') as fd:
        maps = map(_extract_maps, fd.readlines())
    data = filter(lambda x : x['perms'] == 'r-xp', maps)
    return list(data)

def analyze(snapshot_path, maps_path, output_path):
    maps = load_maps(maps_path)
    load_data(snapshot_path, maps, output_path)

def _main():
    parser = argparse.ArgumentParser(description='tracing malloc addr2line.')
    parser.add_argument('files', metavar = 'N', type = str, nargs = '+', help='files')
    parser.add_argument('--maps', dest='maps', type = str, help='cache file of /proc/$PID/maps')

    args = parser.parse_args()
    maps = load_maps(args.maps)

    for file_path in args.files:
        load_data(file_path, maps, '{0}.addr2line'.format(file_path))

if __name__ == '__main__':
    _main()
