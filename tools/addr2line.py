#coding:utf-8 

import sys
import re
import subprocess  
import argparse
import struct

def load_bin_file(file_path):
    file = open(file_path, 'rb')
    fmt = '=qqqq28q' # ptr, time, size, meta, stack
    size = struct.calcsize(fmt)
    data = []
    while True:
        line = file.read(size)
        if not line:
            break
        assert(len(line) == size)
        unpack_data = struct.unpack(fmt, line)
        if unpack_data[0] != 0:
            for index, value in enumerate(reversed(unpack_data)):
                if value != 0:
                    index = len(unpack_data) - index
                    break
            line_data = {'ptr' : unpack_data[0], 'time' : unpack_data[1], 'size' : unpack_data[2], 'meta' : unpack_data[3], 'stack' : unpack_data[4:index]}
            data.append(line_data)
            print('load_bin_file', line_data)
    return data

def save_file(file_path, data):
    file = open(file_path, 'w')
    for item in data:
        file.write('time:{0}\tsize:{1}\tptr:0x{2:x}\tmeta:{3}\n'.format(item['time'], item['size'], item['ptr'], item['meta']))
        for frame in item['stack_line']:
            file.write('{0}\n'.format(frame))
        file.write('========\n\n')

_addr2line_cache = {}
def addr2line(address, fbase, fname):
    if fbase == '(nil)':
        return "{0}\t{1}\t{2}".format(address, fbase, fname)

    if address in _addr2line_cache:
        cache_line = _addr2line_cache[address]
        if fbase == cache_line['fbase'] and fname == cache_line['fname']:
            return cache_line['line']

    address_i = address
    if fbase != 0x400000:
        address_i = address - fbase
    print(address, fbase, fname, address_i)

    pcmd = 'addr2line -f -s -C -e {0} 0x{1:x}'.format(fname, address_i)
    p = subprocess.Popen(pcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    function = p.stdout.readline().rstrip('\n')
    file = p.stdout.readline().rstrip('\n')
    line = "{0}\t{1}\t{2}".format(function, file, fname)

    _addr2line_cache[address] = {'fbase':fbase, 'fname':fname, 'line':line}
    return line

def load_data(file_path, maps):
    data = load_bin_file(file_path)
    data.sort(key = lambda x : x['time'])

    for item in data:
        stack_line = []
        for address in item['stack']:
            line = '0x{:x}'.format(address)
            moudle = list(filter(lambda x : address >= x['start'] and address < x['end'], maps))
            if moudle:
                line = addr2line(address, moudle[0]['start'], moudle[0]['pathname'])
            stack_line.append(line)
        item['stack_line'] = stack_line

    save_file('{0}.addr2line'.format(file_path), data)

def _extract_maps(line):
    maps = re.match(r'([0-9A-Fa-f]+)-([0-9A-Fa-f]+)\s+([-r][-w][-x][sp])\s+([0-9A-Fa-f]+)\s+([:0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+(.*)$', line)
    start = int(maps.group(1), 16)
    end = int(maps.group(2), 16)
    perms = maps.group(3)
    offset = int(maps.group(4), 16)
    dev = maps.group(5)
    inode = maps.group(6)
    pathname = maps.group(7)

    return { 'start': start, 'end': end, 'perms': perms, 'offset': offset, 'dev': dev, 'inode': inode, 'pathname': pathname }

def load_maps(file_path):
    with open(file_path, 'r') as fd:
        maps = map(_extract_maps, fd.readlines())
    data = filter(lambda x : x['perms'] == 'r-xp', maps)
    return data

def main():
    parser = argparse.ArgumentParser(description='tracking malloc addr2line.')
    parser.add_argument('files', metavar = 'N', type = str, nargs = '+', help='files')
    parser.add_argument('--maps', dest='maps', type = str, help='cache file of /proc/$PID/maps')

    args = parser.parse_args()
    maps = load_maps(args.maps)
    print(type(maps), maps)

    for file_path in args.files:
        load_data(file_path, maps)

if __name__ == '__main__':
    main()
