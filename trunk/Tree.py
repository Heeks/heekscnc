import HeeksCNC
import platform
        
class Tree:
    def __init__(self):
        #create self.window and add it to the CAD system
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            import wx
            from wxCAMWindow import CAMWindow

            if platform.system() == "Windows":
                self.window = CAMWindow()
                HeeksCNC.add_window(self.window.GetHandle())
            else:
                ID = HeeksCNC.get_frame_id()
                frame = wx.FindWindowById(ID)
                self.window = CAMWindow(frame)
                HeeksCNC.add_window(self.window.GetId())
            
        elif HeeksCNC.widgets == HeeksCNC.WIDGETS_QT:
            from PyQt4 import QtGui

            app = QtGui.QApplication([])
            self.window = QtGui.QWidget()
            self.window.setWindowTitle('CAM2')
            tree = QtGui.QTreeWidget(self.window)
            tree_item = QtGui.QTreeWidgetItem(tree)
            tree_item.setText(0, "Program")
            tree.addTopLevelItem(tree_item)
            if platform.system() == "Windows":
                HeeksCNC.add_window(self.window.winId())
                self.window.show()
            else:
                #HeeksCNC.add_window(widget.GetId())
                self.window.show()
                
    def Add(self, object):
        # adds a TreeObject to the tree
        self.window.add(object, object.parent)
        
        # add its children too
        if object.children != None:
            for child in object.children:
                self.Add(child)
        
    def Remove(self, object):
        # remove a TreeObject from the tree
        self.window.remove(object)
        
    def Refresh(self):
        self.window.Refresh()
