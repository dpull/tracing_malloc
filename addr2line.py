#coding:utf-8 

import sys
import re
import subprocess  

def load_file(file_path):
    file = open(file_path)
    data = []
    line_data = None
    while True:
        line = file.readline()
        if not line:
            break
        if line.startswith('time:'):
            match = re.match(r'time:(\d+)\tsize:(\d+)\tptr:(0[xX][0-9a-fA-F]+)', line)
            line_data = {'time' : int(match.group(1)), 'size' : int(match.group(2)), 'ptr' : match.group(2), 'stack' : []}
        elif line.startswith('========'):
            data.append(line_data)
        else:
            line_data['stack'].append(line)
    return data

def save_file(file_path, data):
    file = open(file_path, 'w')
    for item in data:
        file.write('time:{0}\tsize:{1}\tptr:{2}\n'.format(item['time'], item['size'], item['ptr']))
        for frame in item['stack_line']:
            file.write('{0}'.format(frame))
        file.write('========\n\n')

def addr2line(exe, addr):
    result = ''
    pcmd = 'addr2line -p -f -i -e {0} {1}'.format(exe, addr)
    p = subprocess.Popen(pcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT);

    while True:
        line = p.stdout.readline();
        if not line:  
            break
        result += line

    if result == '?? ??:0\n':
        return ''
    return result

def proc(file_path):
    data = load_file(file_path)
    for item in data:
        stack_line = []
        for frame in item['stack']:
            match = re.match(r'([\S\/]+)\(([\S\+]*)\) \[(0[xX][0-9a-fA-F]+)\]', frame)
            if not match:
                stack_line.append(frame)
                continue
            exe = match.group(1)
            addr = match.group(3)
            line = addr2line(exe, addr)
            if len(line) > 0:
                stack_line.append(line)
            else:
                stack_line.append(frame)
        item['stack_line'] = stack_line

    save_file('{0}.addr2line'.format(file_path), data)

def main():
    file_paths = sys.argv[1:]
    for file_path in file_paths:
        proc(file_path)

if __name__== "__main__":
    main()
