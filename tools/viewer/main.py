#!/usr/bin/env python
#-*- coding: utf-8 -*-
import view
import data

class Controller:
    def __init__(self):
        self.commond = {
            'Open' : self.Open,
            'SelectCanvas' : self.SelectCanvas,
            'PrevDetail' : self.PrevDetail,
        }
        self.__dataSource = None

    def Open(self, app, **kwargs):
        print("Open", kwargs['FilePath'])
        dataSource = data.DataSource(kwargs['FilePath'])
        self.__dataSource = dataSource
        allocSizeByTime = self.__dataSource.getAllocSizeByTime()
        app.updateCanvas(allocSizeByTime.keys(), allocSizeByTime.values())

    def SelectCanvas(self, app, **kwargs):
        idx = kwargs['Index']
        key, value = self.__dataSource.getAllocSizeByTimeIndex(idx)
        app.updateStatusBar('{0} alloc {1} KB'.format(key, value))

        keys = self.__dataSource.getAllocByTime(key)
        app.updateTreeView(keys)
        app.highlightCanvas(idx)

    def PrevDetail(self, app, **kwargs):
        pass

if __name__ == '__main__':
    ctrl = Controller()
    app = view.Application()
    app.setCommondDispatcher(ctrl.commond)              
    app.mainloop()       