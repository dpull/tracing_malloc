#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys
import re
import datetime  

def load_file(file_path):
    file = open(file_path)
    data = []
    line_data = {}
    while True:
        line = file.readline()
        if not line:
            break
        if line.startswith('time:'):
            match = re.match(r'time:(\d+)\tsize:(\d+)\tptr:(0[xX][0-9a-fA-F]+)', line)
            assert(match)
            line_data = {'time' : int(match.group(1)), 'size' : int(match.group(2)), 'ptr' : match.group(3), 'stack' : []}
        elif line.startswith('========'):
            data.append(line_data)
            line_data = {}
        elif line_data:
            line = line.rstrip('\n')
            line_data['stack'].append(line)
    return data

class DataSource: 
    def __init__(self, file_path):
        self.file_path = file_path
        self.data = load_file(file_path)
        self.timeAllocCache = {}

    def getAllocSizeByTime(self):
        if self.timeAllocCache:
            return self.timeAllocCache

        timeAllocCache = {}
        for item in self.data:
            time = datetime.datetime.fromtimestamp(item['time']).strftime('%H:%M')
            timeAllocCache.setdefault(time, 0)
            timeAllocCache[time] += item['size']

        for k, v in timeAllocCache.items():
            timeAllocCache[k] = v // 1024
            
        self.timeAllocCache = timeAllocCache
        self.timeAllocCacheKeys = list(timeAllocCache.keys())
        return self.timeAllocCache

    def getAllocSizeByTimeIndex(self, idx):
        key = self.timeAllocCacheKeys[idx]
        return key, self.timeAllocCache[key]

    def getAllocByTime(self, key):
        allocInfo = {}
        for item in self.data:
            time = datetime.datetime.fromtimestamp(item['time']).strftime('%H:%M')
            if time == key:
                frame = item['stack'][1]
                allocInfo.setdefault(frame, {'size' : 0, 'items' : []})
                allocInfo[frame]['size'] += item['size']
                allocInfo[frame]['items'].append(item)
        self.allocInfo = allocInfo

        keys = []
        for key, value in self.allocInfo.items():
            info = key.split("\t")
            keys.append([info[0], info[2], value['size']])
        keys.sort(key = lambda x: x[2], reverse = True)       
        return keys