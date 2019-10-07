#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys
import re
import datetime  

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

__ignore_frame_pattern = [
    re.compile(r'[\S\s]*libc\.so(\.\d+)*$'),
    re.compile(r'[\S\s]*libstdc\+\+\.so(\.\d+)*$'),
    re.compile(r'[\S\s]*ld-linux-x86-64\.so(\.\d+)*$'),
    re.compile(r'[\S\s]*libtracking_malloc\.so$'),
]

def is_ignore_frame(frame):
    for pattern in __ignore_frame_pattern:
        if pattern.search(frame):
            return True
    return False

def get_key_frame(item):
    for frame in item['stack']:
        if is_ignore_frame(frame):
            continue
        return frame

def format_bytes(size):
    power = 2**10
    power_labels = ['', 'K', 'M', 'G', 'T']
    n = 0
    while size > power:
        size /= power
        n += 1
    return ('%d%sB' if size == int(size) else '%.2f%sB') % (size, power_labels[n])    

class DataSource: 
    def __init__(self, filePath):
        self.filePath = filePath
        self.rawdata = load_file(filePath)
        self.__procData()

    def __procData(self):
        self.data = {}
        for idx, item in enumerate(self.rawdata):
            timeStr = datetime.datetime.fromtimestamp(item['time']).strftime('%H:%M')
            keyFrame = get_key_frame(item)
            self.data.setdefault(timeStr, dict(size = 0, keyFrame = {}))

            itemData = self.data[timeStr]
            itemData['size'] += item['size']
            itemData['keyFrame'].setdefault(keyFrame, dict(size = 0, rawdata = []))
            
            itemKeyFrameData = itemData['keyFrame'][keyFrame]
            itemKeyFrameData['size'] += item['size']
            itemKeyFrameData['rawdata'].append(idx)

    def getAllocSummary(self):
        summary = {k : v['size'] for k, v in self.data.items()}
        return summary

    def getAllocFrame(self, timeStr):
        itemData = self.data[timeStr]
        keyFrameData = []

        for keyFrame, itemKeyFrameData in itemData['keyFrame'].items():
            function, _, module = keyFrame.split('\t')
            keyFrameData.append(dict(function = function, module = module, size = itemKeyFrameData['size'], keyFrame = keyFrame))
        keyFrameData.sort(key = lambda x : x['size'], reverse = True)       
        return keyFrameData

    def getAllocDetail(self, timeStr, keyFrame):
        itemKeyFrameData = self.data[timeStr]['keyFrame'][keyFrame]
        detail = []
        for rawIdx in itemKeyFrameData['rawdata']:
            item = self.rawdata[rawIdx]
            detail.append(dict(stack = item['stack'], size = item['size']))
        detail.sort(key = lambda x : x['size'], reverse = True)
        return detail