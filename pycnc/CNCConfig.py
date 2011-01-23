import wx

class CNCConfig(wx.Config):
    def __init__(self):
        wx.Config.__init__(self, "HeeksCNC")