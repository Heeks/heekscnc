import HeeksCNC
import platform
        
class Tree:
    def __init__(self):
        #create self.window and add it to the CAD system
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            import wx
            from wxCAMWindow import CAMWindow
            self.window = CAMWindow(HeeksCNC.cad.frame)
            HeeksCNC.cad.add_window(self.window)
            
        elif HeeksCNC.widgets == HeeksCNC.WIDGETS_QT:
            from PyQt4 import QtGui
            self.window = QtGui.QWidget()
            self.window.setWindowTitle('CAM2')
            tree = QtGui.QTreeWidget(self.window)
            tree_item = QtGui.QTreeWidgetItem(tree)
            tree_item.setText(0, "Program")
            tree.addTopLevelItem(tree_item)
            HeeksCNC.cad.add_window(self.window)
            self.window.show()
                
    def Add(self, object):
        # adds a TreeObject to the tree
        self.window.add(object)
        
        # add its children too
        if object.children != None:
            for child in object.children:
                self.Add(child)
        
    def Remove(self, object):
        # remove a TreeObject from the tree
        self.window.remove(object)
        
    def Refresh(self):
        self.window.Refresh()
        
    def AddObjects(objects):
        for object in objects:
            self.window.add(object)

    def RemoveObjects(objects):
        for object in objects:
            self.window.remove(object)