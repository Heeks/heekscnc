import HeeksCNC

class PopupMenu:
    def __init__(self):
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            import wx
            self.menu = wx.Menu()
            self.functions = dict()
            self.next_id = 1

    def AddItem(self, label, func):
        if HeeksCNC.widgets == HeeksCNC.WIDGETS_WX:
            import wx
            id = self.next_id
            self.next_id = self.next_id + 1
            item = self.menu.Append(id, label)
            self.functions[id] = func
            HeeksCNC.cad.frame.Bind(wx.EVT_MENU, self.OnPopupItemSelected, item)

    def OnPopupItemSelected(self, event):
        func = self.functions[event.GetId()]
        func()

