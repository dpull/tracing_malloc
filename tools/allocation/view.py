#!/usr/bin/env python
#-*- coding: utf-8 -*-

import random
import math
import tkinter 
from tkinter import ttk
from tkinter import filedialog
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2Tk)
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
        self.__createTreeView(frame)
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
        count = len(self.__canvasAxesBar)
        if idx < 0 or idx >= count:
            return
        self.__onCommand('SelectCanvas', Index = idx)

    def __delayDrawCanvas(self):
        self.__canvasNeedRender = True
        self.after(10, self.__delayDrawCanvasCallback)

    def __delayDrawCanvasCallback(self):
         if self.__canvasNeedRender: 
             self.__canvasNeedRender = False 
             self.__canvasRender.draw()         

    def updateCanvas(self, x, height):
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

    def __createTreeView(self, root):
        self.__treeview = ttk.Treeview(root)
        self.__treeview['columns'] = ('Module', 'Size')
        self.__treeview.column('#0', width=512, minwidth=256)
        self.__treeview.column('Module', width = 256, minwidth = 128, stretch = tkinter.NO)
        self.__treeview.column('Size', width = 80, minwidth = 50, stretch = tkinter.NO)
        self.__treeview.heading('#0', text = 'Function',anchor = tkinter.W)
        self.__treeview.heading('Module', text='Module',anchor = tkinter.W)
        self.__treeview.heading('Size', text = 'Size',anchor = tkinter.W)
        self.__treeview.pack(side = tkinter.LEFT, fill = tkinter.BOTH, expand = 1)

    def updateTreeView(self, dataList):
        self.__treeview.delete(*self.__treeview.get_children())
        index = 0
        for item in dataList:
            index += 1
            self.__treeview.insert('', index, text = item[0], values=(item[1], item[2]))

    def __createDetail(self, root):
        node = tkinter.Frame(root)
        node.pack(side = tkinter.TOP)
        nodeTop = tkinter.Frame(node)
        nodeTop.pack(side = tkinter.TOP, fill = 'x')
        btnPrev = ttk.Button(nodeTop, text='<<', command = lambda:self.__onCommand('PrevDetail'))
        btnPrev.pack(side = tkinter.LEFT)
        btnNext = ttk.Button(nodeTop, text='>>', command = lambda:self.__onCommand('NextDetail'))
        btnNext.pack(side = tkinter.RIGHT)
        self.__detail = ttk.Label(node, anchor = tkinter.NW)
        self.__detail.pack(side = tkinter.TOP, fill = tkinter.BOTH, expand = 1)

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
        self.updateTreeView()
        self.__detail['text'] = "Test\n\nTest1"      

if __name__ == '__main__':
    app = Application()      
    app.mainloop()       