import wx

class PictureWindow(wx.Window):
    def __init__(self, parent, size = None, bitmap = None):# use either size or bitmap
        if bitmap != None:
            wx.Window.__init__(self, parent, size = wx.Size(b2.GetWidth(), b2.GetHeight()))
        elif size != None:
            wx.Window.__init__(self, parent, size = size)
        else:
            wx.Window.__init__(self, parent)
        self.bitmap = bitmap
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        
    def OnPaint(self, event):
        dc = wx.PaintDC(self)
        if self.bitmap != None:
            dc.DrawBitmap(self.bitmap, 0, 0, False)
            
    def SetPicture(self, bitmap):
        self.bitmap = bitmap
        self.Refresh()
        
    def SetPictureBitmap(self, bitmap, filepath, image_type):
        if bitmap == None:
            bitmap = wx.BitmapFromImage(wx.Image(filepath, image_type))
        self.SetPicture(bitmap)
