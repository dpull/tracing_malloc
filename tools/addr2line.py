#coding:utf-8 

import sys
import re
import subprocess  

def load_file(file_path):
    file = open(file_path)
    data = []
    line_data = {}
    pattern = re.compile(r'time:(\d+)\tsize:(\d+)\tptr:(0[xX][0-9a-fA-F]+)')
    while True:
        line = file.readline()
        if not line:
            break
        if line.startswith('time:'):
            match = pattern.match(line)
            assert(match)
            line_data = {'time' : int(match.group(1)), 'size' : int(match.group(2)), 'ptr' : match.group(3), 'stack' : []}
        elif line.startswith('========'):
            data.append(line_data)
            line_data = {}
        elif line_data:
            line = line.rstrip('\n')
            line_data['stack'].append(line)
    return data

def save_file(file_path, data):
    file = open(file_path, 'w')
    for item in data:
        file.write('time:{0}\tsize:{1}\tptr:{2}\n'.format(item['time'], item['size'], item['ptr']))
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

    address_i = int(address, 16)
    fbase_i = int(fbase, 16)
    if fbase_i != 0x400000:
        address_i = address_i - fbase_i

    pcmd = 'addr2line -f -s -C -e {0} 0x{1:x}'.format(fname, address_i)
    p = subprocess.Popen(pcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    function = p.stdout.readline().rstrip('\n')
    file = p.stdout.readline().rstrip('\n')
    line = "{0}\t{1}\t{2}".format(function, file, fname)

    _addr2line_cache[address] = {'fbase':fbase, 'fname':fname, 'line':line}
    return line

def proc(file_path):
    data = load_file(file_path)
    data.sort(key = lambda x : x['time'])

    for item in data:
        stack_line = []
        for frame in item['stack']:
            match = frame.split('\t')
            assert(len(match) == 3)
            line = addr2line(match[0], match[1], match[2])
            stack_line.append(line)
        item['stack_line'] = stack_line

    save_file('{0}.addr2line'.format(file_path), data)

def main():
    file_paths = sys.argv[1:]
    for file_path in file_paths:
        proc(file_path)

if __name__ == '__main__':
    main()
