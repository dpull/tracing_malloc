#!/usr/bin/env python
#-*- coding: utf-8 -*-
import view
import data

class Controller:
    def __init__(self):
        self.commond = {
            'Open' : self.Open,
            'SelectCanvas' : self.SelectCanvas,
            'SelectTreeview' : self.SelectTreeview,
            'ShowDetail' : self.ShowDetail,
        }
        self.__dataSource = None

    def Open(self, app, **kwargs):
        print("Open", kwargs['FilePath'])
        dataSource = data.DataSource(kwargs['FilePath'])
        self.__dataSource = dataSource
        self.__allocSummary = self.__dataSource.getAllocSummary()
        app.updateCanvas(self.__allocSummary.keys(), self.__allocSummary.values())
        app.updateTreeView([])

    def SelectCanvas(self, app, **kwargs):
        timeStr = kwargs['TimeStr']
        size = data.format_bytes(self.__allocSummary[timeStr])
        app.updateStatusBar('{0} alloc {1}.'.format(timeStr, size))

        keyFrameData = self.__dataSource.getAllocFrame(timeStr)
        for v in keyFrameData:
            v['size'] = data.format_bytes(v['size'])
        app.updateTreeView(keyFrameData)

    def SelectTreeview(self, app, **kwargs):
        keyFrame = kwargs['KeyFrame']
        self.__allocDetail = self.__dataSource.getAllocDetail(app.LastSelectTimeStr, keyFrame)
        for v in self.__allocDetail:
            v['size'] = data.format_bytes(v['size'])
        app.updateDetail(0, len(self.__allocDetail), self.__allocDetail[0])

    def ShowDetail(self, app, **kwargs):
        idx = kwargs['Index']
        app.updateDetail(idx, len(self.__allocDetail), self.__allocDetail[idx])

if __name__ == '__main__':
    ctrl = Controller()
    app = view.Application()
    app.setCommondDispatcher(ctrl.commond)              
    app.mainloop()       