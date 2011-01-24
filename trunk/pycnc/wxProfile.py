# -*- coding: utf-8 -*- 

###########################################################################
## Python code generated with wxFormBuilder (version Nov 18 2010)
## http://www.wxformbuilder.org/
##
## PLEASE DO "NOT" EDIT THIS FILE!
###########################################################################

import wx

###########################################################################
## Class Profiledlg
###########################################################################

class Profiledlg ( wx.Dialog ):
	
	def __init__( self, parent ):
		wx.Dialog.__init__ ( self, parent, id = wx.ID_ANY, title = u"Profile Milling", pos = wx.DefaultPosition, size = wx.Size( 625,812 ), style = wx.DEFAULT_DIALOG_STYLE )
		
		self.SetSizeHintsSz( wx.DefaultSize, wx.DefaultSize )
		
		bSizer1 = wx.BoxSizer( wx.VERTICAL )
		
		gSizer13 = wx.GridSizer( 2, 2, 0, 0 )
		
		sbSizer8 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Profiles" ), wx.VERTICAL )
		
		bSizer36 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText30 = wx.StaticText( self, wx.ID_ANY, u"Pick Profile", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText30.Wrap( -1 )
		bSizer36.Add( self.m_staticText30, 0, wx.ALIGN_CENTER_VERTICAL, 5 )
		
		self.m_filePicker2 = wx.FilePickerCtrl( self, wx.ID_ANY, wx.EmptyString, u"Select a file", u"*.*", wx.DefaultPosition, wx.DefaultSize, wx.FLP_SAVE|wx.FLP_USE_TEXTCTRL )
		bSizer36.Add( self.m_filePicker2, 0, wx.ALL, 5 )
		
		sbSizer8.Add( bSizer36, 0, wx.ALIGN_RIGHT, 5 )
		
		bSizer37 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText47 = wx.StaticText( self, wx.ID_ANY, u"Profiles", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText47.Wrap( -1 )
		bSizer37.Add( self.m_staticText47, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl41 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer37.Add( self.m_textCtrl41, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		sbSizer8.Add( bSizer37, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer431 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_checkBox61 = wx.CheckBox( self, wx.ID_ANY, u"Auto Roll On", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_checkBox61.SetValue(True) 
		bSizer431.Add( self.m_checkBox61, 0, 0, 5 )
		
		self.m_checkBox711 = wx.CheckBox( self, wx.ID_ANY, u"Auto Roll Off", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_checkBox711.SetValue(True) 
		bSizer431.Add( self.m_checkBox711, 0, 0, 5 )
		
		sbSizer8.Add( bSizer431, 0, wx.ALIGN_RIGHT, 5 )
		
		bSizer452 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText352 = wx.StaticText( self, wx.ID_ANY, u"Roll On Radius", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText352.Wrap( -1 )
		bSizer452.Add( self.m_staticText352, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_roll_radius = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer452.Add( self.m_roll_radius, 0, wx.ALL, 5 )
		
		sbSizer8.Add( bSizer452, 0, wx.ALIGN_RIGHT, 5 )
		
		bSizer4511 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText3511 = wx.StaticText( self, wx.ID_ANY, u"Roll Off Radius", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText3511.Wrap( -1 )
		bSizer4511.Add( self.m_staticText3511, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl3011 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer4511.Add( self.m_textCtrl3011, 0, wx.ALL, 5 )
		
		sbSizer8.Add( bSizer4511, 0, wx.ALIGN_RIGHT, 5 )
		
		gSizer13.Add( sbSizer8, 0, wx.EXPAND, 5 )
		
		bSizer58 = wx.BoxSizer( wx.VERTICAL )
		
		self.m_bitmap3 = wx.StaticBitmap( self, wx.ID_ANY, wx.Bitmap( u"../bitmaps/profile/general.png", wx.BITMAP_TYPE_ANY ), wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer58.Add( self.m_bitmap3, 0, wx.ALL, 5 )
		
		gSizer13.Add( bSizer58, 1, wx.EXPAND, 5 )
		
		sbSizer1 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Z Levels" ), wx.VERTICAL )
		
		gSizer3 = wx.GridSizer( 4, 1, 0, 0 )
		
		bSizer2 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.clearance_height  = wx.StaticText( self, wx.ID_ANY, u"Clearance Height", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.clearance_height .Wrap( -1 )
		bSizer2.Add( self.clearance_height , 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_clearance_height  = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer2.Add( self.m_clearance_height , 0, wx.ALL, 5 )
		
		gSizer3.Add( bSizer2, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer3 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.rapid_down_to_height = wx.StaticText( self, wx.ID_ANY, u"Rapid to Z", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.rapid_down_to_height.Wrap( -1 )
		bSizer3.Add( self.rapid_down_to_height, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_rapid_down_to_height = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer3.Add( self.m_rapid_down_to_height, 0, wx.ALL, 5 )
		
		gSizer3.Add( bSizer3, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer4 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.start_depth  = wx.StaticText( self, wx.ID_ANY, u"Start Depth", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.start_depth .Wrap( -1 )
		bSizer4.Add( self.start_depth , 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_start_depth  = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer4.Add( self.m_start_depth , 0, wx.ALL, 5 )
		
		gSizer3.Add( bSizer4, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer5 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.final_depth = wx.StaticText( self, wx.ID_ANY, u"Final Depth", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.final_depth.Wrap( -1 )
		bSizer5.Add( self.final_depth, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_final_depth = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer5.Add( self.m_final_depth, 0, wx.ALL, 5 )
		
		gSizer3.Add( bSizer5, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer6 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.step_down = wx.StaticText( self, wx.ID_ANY, u"Step Down", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.step_down.Wrap( -1 )
		bSizer6.Add( self.step_down, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_step_down = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer6.Add( self.m_step_down, 0, wx.ALL, 5 )
		
		gSizer3.Add( bSizer6, 1, wx.ALIGN_RIGHT, 5 )
		
		self.m_checkBox7 = wx.CheckBox( self, wx.ID_ANY, u"Save Values", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_checkBox7.SetValue(True) 
		gSizer3.Add( self.m_checkBox7, 0, wx.ALIGN_RIGHT, 5 )
		
		sbSizer1.Add( gSizer3, 1, wx.EXPAND, 5 )
		
		gSizer13.Add( sbSizer1, 0, wx.EXPAND, 5 )
		
		sbSizer7 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Tool" ), wx.VERTICAL )
		
		gSizer12 = wx.GridSizer( 3, 1, 0, 0 )
		
		bSizer39 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText31 = wx.StaticText( self, wx.ID_ANY, u"Tool", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText31.Wrap( -1 )
		bSizer39.Add( self.m_staticText31, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		m_choice4Choices = []
		self.m_choice4 = wx.Choice( self, wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_choice4Choices, 0 )
		self.m_choice4.SetSelection( 0 )
		bSizer39.Add( self.m_choice4, 0, wx.ALL, 5 )
		
		gSizer12.Add( bSizer39, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer301 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText261 = wx.StaticText( self, wx.ID_ANY, u"Tool On Side", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText261.Wrap( -1 )
		bSizer301.Add( self.m_staticText261, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		m_choice11Choices = [ u"On", u"Left", u"Right" ]
		self.m_choice11 = wx.Choice( self, wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_choice11Choices, 0 )
		self.m_choice11.SetSelection( 0 )
		bSizer301.Add( self.m_choice11, 0, wx.ALL, 5 )
		
		gSizer12.Add( bSizer301, 0, wx.ALIGN_RIGHT, 5 )
		
		bSizer30 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText26 = wx.StaticText( self, wx.ID_ANY, u"Path Direction", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText26.Wrap( -1 )
		bSizer30.Add( self.m_staticText26, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		m_choice1Choices = [ u"Climb", u"Conventional" ]
		self.m_choice1 = wx.Choice( self, wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_choice1Choices, 0 )
		self.m_choice1.SetSelection( 0 )
		bSizer30.Add( self.m_choice1, 0, wx.ALL, 5 )
		
		gSizer12.Add( bSizer30, 0, wx.ALIGN_RIGHT, 5 )
		
		bSizer35 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText29 = wx.StaticText( self, wx.ID_ANY, u"Extra Offset", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText29.Wrap( -1 )
		bSizer35.Add( self.m_staticText29, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl26 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer35.Add( self.m_textCtrl26, 0, wx.ALL, 5 )
		
		gSizer12.Add( bSizer35, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer351 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText291 = wx.StaticText( self, wx.ID_ANY, u"Spindle Speed", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText291.Wrap( -1 )
		bSizer351.Add( self.m_staticText291, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl261 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer351.Add( self.m_textCtrl261, 0, wx.ALL, 5 )
		
		gSizer12.Add( bSizer351, 1, wx.ALIGN_RIGHT, 5 )
		
		sbSizer7.Add( gSizer12, 0, wx.ALIGN_RIGHT, 5 )
		
		gSizer13.Add( sbSizer7, 0, wx.EXPAND, 5 )
		
		bSizer70 = wx.BoxSizer( wx.VERTICAL )
		
		gSizer13.Add( bSizer70, 1, wx.EXPAND, 5 )
		
		bSizer60 = wx.BoxSizer( wx.VERTICAL )
		
		bSizer61 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText43 = wx.StaticText( self, wx.ID_ANY, u"ABS/INC Mode", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText43.Wrap( -1 )
		bSizer61.Add( self.m_staticText43, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		m_choice5Choices = [ u"Absolute", u"Incremental" ]
		self.m_choice5 = wx.Choice( self, wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_choice5Choices, 0 )
		self.m_choice5.SetSelection( 0 )
		bSizer61.Add( self.m_choice5, 0, wx.ALL, 5 )
		
		bSizer60.Add( bSizer61, 0, wx.ALIGN_RIGHT|wx.ALL, 5 )
		
		bSizer66 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText44 = wx.StaticText( self, wx.ID_ANY, u"horizontal feed rate", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText44.Wrap( -1 )
		bSizer66.Add( self.m_staticText44, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl37 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer66.Add( self.m_textCtrl37, 0, wx.ALL, 5 )
		
		bSizer60.Add( bSizer66, 0, wx.ALIGN_RIGHT|wx.ALL, 5 )
		
		bSizer67 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText45 = wx.StaticText( self, wx.ID_ANY, u"vertical feed rate", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText45.Wrap( -1 )
		bSizer67.Add( self.m_staticText45, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl38 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer67.Add( self.m_textCtrl38, 0, wx.ALL, 5 )
		
		bSizer60.Add( bSizer67, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer68 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_staticText46 = wx.StaticText( self, wx.ID_ANY, u"comment", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText46.Wrap( -1 )
		bSizer68.Add( self.m_staticText46, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
		
		self.m_textCtrl39 = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_textCtrl39.SetMinSize( wx.Size( 200,-1 ) )
		
		bSizer68.Add( self.m_textCtrl39, 0, wx.ALL, 5 )
		
		bSizer60.Add( bSizer68, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer711 = wx.BoxSizer( wx.HORIZONTAL )
		
		self.m_checkBox121 = wx.CheckBox( self, wx.ID_ANY, u"Active", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_checkBox121.SetValue(True) 
		bSizer711.Add( self.m_checkBox121, 0, wx.ALL, 5 )
		
		self.m_checkBox1311 = wx.CheckBox( self, wx.ID_ANY, u"Visible", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_checkBox1311.SetValue(True) 
		bSizer711.Add( self.m_checkBox1311, 0, wx.ALL, 5 )
		
		bSizer60.Add( bSizer711, 1, wx.ALIGN_RIGHT, 5 )
		
		bSizer69 = wx.BoxSizer( wx.VERTICAL )
		
		m_sdbSizer6 = wx.StdDialogButtonSizer()
		self.m_sdbSizer6OK = wx.Button( self, wx.ID_OK )
		m_sdbSizer6.AddButton( self.m_sdbSizer6OK )
		self.m_sdbSizer6Cancel = wx.Button( self, wx.ID_CANCEL )
		m_sdbSizer6.AddButton( self.m_sdbSizer6Cancel )
		m_sdbSizer6.Realize();
		bSizer69.Add( m_sdbSizer6, 1, wx.EXPAND, 5 )
		
		bSizer60.Add( bSizer69, 1, wx.EXPAND, 5 )
		
		gSizer13.Add( bSizer60, 1, wx.EXPAND, 5 )
		
		bSizer1.Add( gSizer13, 0, wx.EXPAND, 5 )
		
		self.SetSizer( bSizer1 )
		self.Layout()
		
		self.Centre( wx.BOTH )
		
		# Connect Events
		self.Bind( wx.EVT_CLOSE, self.CloseWindow )
		self.m_sdbSizer6Cancel.Bind( wx.EVT_BUTTON, self.CloseWindow )
		self.m_sdbSizer6OK.Bind( wx.EVT_BUTTON, self.CloseWindow )
	
	def __del__( self ):
		pass
	
	
	# Virtual event handlers, overide them in your derived class
	def CloseWindow( self, event ):
		event.Skip()
	
	
	

