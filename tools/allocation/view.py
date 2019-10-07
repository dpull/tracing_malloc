#!/usr/bin/env python
#-*- coding: utf-8 -*-

import random
import math
import tkinter 
from tkinter import ttk
from tkinter import filedialog
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.figure

class Application(tkinter.Frame):    
    def __init__(self, master = None):
        tkinter.Frame.__init__(self, master)   
        self.master.title('Tracking malloc')    
        self.master.state("zoomed")
        self.__createWidgets()
        self.__cmdDispatcher = {}

    def __createWidgets(self):
        self.__createMenu()
        self.__createStatusBar()

        root = tkinter.PanedWindow(self.master, borderwidth = 2, orient = tkinter.VERTICAL, showhandle = True)
        root.pack(side = tkinter.TOP, fill = tkinter.BOTH, expand = 1)
        self.__createCanvas(root)

        frame = tkinter.Frame(root, borderwidth = 1)
        root.add(frame)
        self.__createFrameList(frame)
        self.__createDetail(frame)

    def __createMenu(self):
        menu = tkinter.Menu(self.master)
        fileMenu = tkinter.Menu(menu, tearoff = 0)
        self.master.config(menu = menu)
        menu.add_cascade(label = 'File', menu = fileMenu)
        fileMenu.add_command(label = 'Open', command = self.__showOpenFileDialog)
        fileMenu.add_command(label = 'Exit', command = self.master.quit)

    def __showOpenFileDialog(self):
        ftypes = [('Trace stack files', '*.addr2line'), ('All files', '*')]
        dlg = filedialog.Open(self, filetypes = ftypes)
        path = dlg.show()
        if path :
            self.__onCommand('Open', FilePath = path)

    def __createStatusBar(self):
        self.__statusBar = tkinter.Label(self.master, text = "…", relief = tkinter.SUNKEN, anchor= tkinter.W)
        self.__statusBar.pack(side = tkinter.BOTTOM, fill = tkinter.X)

    def updateStatusBar(self, label):
        self.__statusBar['text'] = label        

    def __createCanvas(self, root):
        canvas = tkinter.Frame(root)
        root.add(canvas)

        fig = matplotlib.figure.Figure(figsize = (6, 2), dpi = 100)
        self.__canvasAxes = fig.add_subplot(111)
        self.__canvasAxes.set_xlabel('Time')
        self.__canvasAxes.set_ylabel('Bytes')
        self.__canvasAxesBar = self.__canvasAxes.bar([], [])
        self.__canvasRender = FigureCanvasTkAgg(fig, master = canvas)  
        self.__canvasRender.draw()
        self.__canvasRender.get_tk_widget().pack(fill = tkinter.BOTH, expand = 1)
        self.__canvasRender.mpl_connect('button_press_event', self.__onCanvasButtonPress)

    def __onCanvasButtonPress(self, event):
        if not event.xdata:
            return
        xdataDelta = event.xdata - math.floor(event.xdata)
        if xdataDelta < 0.6 and  xdataDelta > 0.4:
            return
        idx = math.floor(event.xdata + 0.5)
        count = len(self.__canvasAxesBarX)
        if idx < 0 or idx >= count:
            return

        self.LastSelectTimeStr = self.__canvasAxesBarX[idx]
        self.highlightCanvas(idx)
        self.__onCommand('SelectCanvas', TimeStr = self.LastSelectTimeStr)

    def __delayDrawCanvas(self):
        self.__canvasNeedRender = True
        self.after(10, self.__delayDrawCanvasCallback)

    def __delayDrawCanvasCallback(self):
        if self.__canvasNeedRender: 
            self.__canvasNeedRender = False 
            self.__canvasRender.draw()         

    def updateCanvas(self, x, height):
        self.__canvasAxesBarX = list(x)
        self.__canvasAxes.clear()
        self.__canvasAxesBar = self.__canvasAxes.bar(x, height)
        self.__delayDrawCanvas()

    def highlightCanvas(self, highlightIdx):
        count = len(self.__canvasAxesBar)
        if count <= highlightIdx:
            return

        for i in range(count):
            self.__canvasAxesBar[i].set_color('lightsteelblue' if i != highlightIdx else 'steelblue')
        self.__delayDrawCanvas()      

    def __createFrameList(self, root):
        self.__frameList = ttk.Treeview(root)
        self.__frameList['columns'] = ('Module', 'Size')
        self.__frameList.column('#0', width=512, minwidth=256)
        self.__frameList.column('Module', width = 256, minwidth = 128, stretch = tkinter.NO)
        self.__frameList.column('Size', width = 80, minwidth = 50, stretch = tkinter.NO)
        self.__frameList.heading('#0', text = 'Function',anchor = tkinter.W)
        self.__frameList.heading('Module', text='Module',anchor = tkinter.W)
        self.__frameList.heading('Size', text = 'Size',anchor = tkinter.W)
        self.__frameList.pack(side = tkinter.LEFT, fill = tkinter.BOTH, expand = 1)
        self.__frameList.bind('<<TreeviewSelect>>', self.__selectFrameList)

    def __selectFrameList(self, event):
        tags = self.__frameList.item(self.__frameList.focus(), 'tags')
        keyFrame = tags[0]
        self.__onCommand('SelectTreeview', KeyFrame = keyFrame)

    def updateFrameList(self, dataList):
        self.__frameList.delete(*self.__frameList.get_children())
        for idx, item in enumerate(dataList):
            self.__frameList.insert('', idx + 1, text = item['function'], values=(item['module'], item['size']), tags = [item['keyFrame']])

    def __createDetail(self, root):
        node = tkinter.Frame(root)
        node.pack(side = tkinter.TOP)
        nodeTop = tkinter.Frame(node)
        nodeTop.pack(side = tkinter.TOP, fill = 'x')

        self.__detailPrev = ttk.Button(nodeTop, text='<<', command = lambda:self.__onCommand('ShowDetail', Index = self.__detailIndex - 1))
        self.__detailPrev.pack(side = tkinter.LEFT)

        self.__detailSummary = ttk.Label(nodeTop, text = '100/100')
        self.__detailSummary.pack(side = tkinter.LEFT, expand = 1)

        self.__detailNext = ttk.Button(nodeTop, text='>>', command = lambda:self.__onCommand('ShowDetail', Index = self.__detailIndex + 1))
        self.__detailNext.pack(side = tkinter.RIGHT)

        self.__detailStack = ttk.Treeview(node)
        self.__detailStack['columns'] = ('File', 'Module')
        self.__detailStack.column('#0', width=256, minwidth=32)
        self.__detailStack.column('File', width = 128, minwidth = 32, stretch = tkinter.NO)
        self.__detailStack.column('Module', width = 64, minwidth = 32, stretch = tkinter.NO)
        self.__detailStack.heading('#0', text = 'Function',anchor = tkinter.W)
        self.__detailStack.heading('File', text = 'File',anchor = tkinter.W)
        self.__detailStack.heading('Module', text='Module',anchor = tkinter.W)
        self.__detailStack.pack(side = tkinter.TOP, fill = tkinter.BOTH, expand = 1)

    def updateDetail(self, idx, count, detail):
        self.__detailIndex = idx
        self.__detailSummary['text'] = 'Size:{}({}/{})'.format(detail['size'], idx + 1, count)
        self.__detailPrev['state'] = tkinter.NORMAL if idx > 0 else tkinter.DISABLED
        self.__detailNext['state'] = tkinter.NORMAL if idx < (count - 1) else tkinter.DISABLED

        self.__detailStack.delete(*self.__detailStack.get_children())
        for idx, line in enumerate(detail['stack']):
            function, fileName, module = line.split('\t')
            self.__detailStack.insert('', idx + 1, text = function, values=(fileName, module))

    def setCommondDispatcher(self, dispatcher):
        self.__cmdDispatcher = dispatcher

    def __onCommand(self, cmd, **kwargs):
        if self.__cmdDispatcher:
            dispatcher = self.__cmdDispatcher.get(cmd)
            if not dispatcher:
                return
            dispatcher(self, **kwargs)
        else:
            self.__runTest(cmd, **kwargs)

    def __runTest(self, cmd, **kwargs):
        print('__onCommand', cmd)
        self.updateCanvas(['16:0{}'.format(x) for x in range(20)], [random.randint(10, 20) for x in range(20)])
        self.highlightCanvas(2)
        self.updateFrameList([])

if __name__ == '__main__':
    app = Application()      
    app.mainloop()       