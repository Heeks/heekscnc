#!/usr/bin/python
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""
@newfield purpose: Purpose

@purpose:  Read EMC tool table of various shapes and forms

See also:
C code at http://git.linuxcnc.org/gitweb?p=emc2.git;a=blob;f=src/emc/iotask/ioControl.cc;h=c9730c95ff61a82ab07eb5e64f518fcd6c876b43;hb=tlo_all_axes
tool table example at: http://git.linuxcnc.org/gitweb?p=emc2.git;a=blob;f=configs/sim/sim.tbl;hb=tlo_all_axes

New format (Michael G, tlo_all_axes)

T1 P1 D0.125000 Z+0.511000 ;1/8 end mill
T2 P2 D0.062500 Z+0.100000 ;1/16 end mill
T3 P3 D0.201000 Z+1.273000 ;#7 tap drill
T99999 P99999 Z+0.100000 ;big tool number

Mill format:
TOOLNO POCKET       LENGTH    DIAMETER  COMMENT

     1      1    +0.511000    0.125000  1/8 end mill
     2      2    +0.100000    0.062500  1/16 end mill
     3      3    +1.273000    0.201000  #7 tap drill
 99999  99999    +0.100000    0.000000  big tool number

Lathe format:
TOOLNO POCKET      ZOFFSET      XOFFSET   DIAMETER  FRONTANGLE   BACKANGLE  ORIENT  COMMENT

     1      1    +0.100000    +0.000000   0.100000  +95.000000 +155.000000       1  
     2      2    +0.000000    +0.000000   0.100000  +85.000000  +25.000000       2  
 



@author: Michael Haberler 
@since:  22.10.2010
@license: GPL
"""

import re
import os,sys
from ConfigParser import ConfigParser
from xml.sax.saxutils import escape


class Tool(object):
    """
    an EMC tool table entry
    
    attributes may be accessed either as self.values[<code>] as per tlo_all_axes format or 
    
    self.tool_number
    self.pocket
    self.diameter
    self.offset_{x,y,z,a,,b,c,u,v,w}
    self.frontangle
    self.backangle
    self.orientation
    (read only)
    """


    tt_codes = 'TPDXYZABCUVWIJQ'
    tt_types = 'IIFFFFFFFFFFFFI'
    intfmt = "%d"
    floatfmt = "%.2f"
    heeksNames = { 'T' : 'tool_number', 'P' : 'pocket_number', 'D':'diameter',
                   'X' : 'tool_length_offset_x', 'Y' : 'tool_length_offset_y',
                   'Z' : 'tool_length_offset', # NB!!
                   'A' : 'tool_length_offset_a',
                   'B' : 'tool_length_offset_b','C' : 'tool_length_offset_c',
                   'U' : 'tool_length_offset_u', 'V' : 'tool_length_offset_v',
                   'W' : 'tool_length_offset_w', 
                   'I' : 'front_angle','J' : 'back_angle', 'Q' : 'orientation'}

    def __init__(self,format=None,units='mm',lathe_fields = 'DXZIJQ',mill_fields='DZ'):
        self.values = dict()
        self.comment = None
        self.format = format
        self.ht = None
        self.units = units
        self.lathe_fields = lathe_fields
        self.mill_fields = mill_fields
        for i in list(self.tt_codes):
            self.values[i] = None
            
    def typeof(self,letter):
        return  Tool.tt_types[Tool.tt_codes.index(letter)]

    @property
    def heeks_tool(self):
        xml = '<Tool title="' 
        if self.comment:
            xml += escape(self.comment) + '" '
            
        else:
            if self.diameter:
                xml += (self.floatfmt + ' %s ') % (self.diameter,self.units )
            xml += '%s tool" ' % (self.format)
            
        xml += 'tool_number="' + str(self.tool_number) + '" id="' + str(self.tool_number) +'">\n'
        xml += '\t<params automatically_generate_title="0" '
        if self.format is 'lathe':
            xml += 'type="6" '
        else:
            xml += 'type="2" '
        for k in list(self.lathe_fields if self.format is 'lathe' else self.mill_fields):
            if self.values[k]:
                try:
                    xml += self.heeksNames[k] + '="' + str(self.values[k]) + '" '
                    
                except KeyError:
                    print "foo"
            
        xml += '/>\n</Tool>\n'
        return xml
 
    @property
    def tool_number(self):
        return self.values['T']
        
    @property
    def pocket(self):
        return self.values['P']
    
    @property
    def diameter(self):
        return self.values['D']
    
    @property
    def offset_x(self):
        return self.values['X']
    
    @property
    def offset_y(self):
        return self.values['Y']
    
    @property
    def offset_z(self):
        return self.values['Z']
    
    @property
    def offset_a(self):
        return self.values['A']
    
    @property
    def offset_b(self):
        return self.values['B']
    
    @property
    def offset_c(self):
        return self.values['C']
    
    @property
    def offset_u(self):
        return self.values['U']
    
    @property
    def offset_v(self):
        return self.values['V']
    
    @property
    def offset_w(self):
        return self.values['W']
    
    @property
    def frontangle(self):
        return self.values['I']
    
    @property
    def backangle(self):
        return self.values['J']
    
    @property
    def orientation(self):
        return self.values['Q']
    
   
    def __str__(self):
        """ 
        return the tooltable entry in tlo_all_axes format as a single line
        """
        s = ""
        for k in self.tt_codes:
            if self.values[k]:
                if self.typeof(k) == 'I':
                    s += k + Tool.intfmt % (self.values[k]) + " "
                if self.typeof(k) == 'F':
                    s += k + Tool.floatfmt % (self.values[k]) + " "
        if self.comment:
            s += "; " + self.comment
        return s
    
class EmcToolTable(object):
    
    def __init__(self,filename=None, emc_inifile=None,units='mm'):
        """
        read and parse an EMC tool table.
        
        If given a filename, read that. 
        
        If passed an EMC INI file, obtain the TOOL_TABLE variable from the
        EMCIO section and use that, prepending the inifile directory if the tool table filename is relative.
        In this case, also remember the units entry from section TRAJ, variable LINEAR_UNITS.
        
        Iterators:
            entries
            mill_table_entries
            lathe_table_entries
        
        """
        self.entries = []
        self.format = None
        self.number = re.compile('([-+]?(\d+(\.\d*)?|\.\d+))')  # G-Code number
        self.units = units
        
        if emc_inifile:
            config = ConfigParser()
            config.read([emc_inifile])
            self.filename = config.get('EMCIO', 'TOOL_TABLE')
            self.units = config.get('TRAJ','LINEAR_UNITS')
            if not os.path.isabs(self.filename):
                self.filename = os.path.join(os.path.dirname(emc_inifile),self.filename)
        else:
            self.filename = filename

        fp = open(self.filename)
        lno = 0        
        for line in fp.readlines():
            lno += 1
            if not line.startswith(';'):   
                if line.strip():
                    entry = self._parseline(lno,line.strip())
                    if entry:
                        self.entries.append(entry)

    
    def _parseline(self,lineno,line):
        """
        read a tooltable line
        if an entry was parsed successfully, return a  Tool() instance
        """     
        comment = None

        if re.match('\A\s*(TOOLNO|POC)\s+(POCKET|FMS)\s+(LEN|LENGTH)\s+(DIAM|DIAMETER)\s+COMMENT\s*\Z',line):
            self.format = 'mill'
            return None

        if re.match('\A\s*(TOOLNO|POC)\s+(POCKET|FMS)\s+ZOFFSET\s+XOFFSET\s+DIAMETER\s+FRONTANGLE\s+BACKANGLE\s+ORIENT\s+COMMENT\s*\Z',line):
            self.format = 'lathe'
            return None

        if re.match('\A\s*T\d+',line): # an MG line
            semi = line.find(";")
            if semi >= 0:
                comment = line[semi+1:]
            entry = line.split(';')[0]
            tt = Tool()
            for field in entry.split():    
                result = re.search('(?P<opcode>[a-zA-Z])(?P<value>[+-]?\d*\.?\d*)',field)
                if result:
                    self._assign(tt, result.group('opcode').capitalize(), result.group('value'))
                else:
                    print "%s:%d  bad line: '%s' " % (self.filename, lineno, entry)
                    
                if tt.values['I'] and tt.values.has_key['J']:   # has frontangle and backangle
                    tt.format = "lathe"
                else:
                    tt.format = "mill"            
            tt.comment = comment
            return tt

        if self.format == 'mill':
            tt = Tool(format=self.format)
            for n,m in enumerate(re.finditer(self.number, line)):
                self._assign(tt, 'TPZD'[n], m.group(0))
                if n == 3:
                    tt.comment = line[m.end():].lstrip()
                    return tt
            return None
        
        if self.format == 'lathe':
            tt = Tool(format=self.format)
            for n,m in enumerate(re.finditer(self.number, line)):
                self._assign(tt, 'TPZXDIJQ'[n], m.group(0))
                if n == 7:
                    tt.comment = line[m.end():].lstrip()
                    return tt
                
        print "%s:%d: unrecognized tool table entry   '%s'" % (self.filename,lineno,line)

    def _assign(self,tt_entry,opcode,value):
        
        if tt_entry.typeof(opcode) == 'I':
            tt_entry.values[opcode] = int(value)
            
        if tt_entry.typeof(opcode) == 'F':
            tt_entry.values[opcode] = float(value)        

    def entries(self):
        for e in self.entries:
            yield e
    
    def lathe_entries(self):
        for e  in [x for x in self.entries if x.format == 'lathe']:
            yield e
    
    def mill_entries(self):
        for e  in [x for x in self.entries if x.format == 'mill']:
            yield e  
            
            
    def toolbynumber(self,number):
        for t in self.entries:
            if t.tool_number == number:
                return t
        raise ValueError,"no tool %d" % (number)
              
    def toolbypocket(self,pocket):
        for t in self.entries:
            if t.pocket == pocket:
                return t
        raise ValueError,"no tool in pocket %d" % (pocket)
              
                         
if __name__ == '__main__':
    
    if len(sys.argv) > 1:
        fn = sys.argv[1]
        if fn.endswith('.ini'):
            tool_table = EmcToolTable(emc_inifile=fn)
        else:
            if len(sys.argv) == 3:
                tool_table = EmcToolTable(filename=sys.argv[1], units=sys.argv[2])
            else:
                tool_table = EmcToolTable(filename=sys.argv[1])
    else:
        print >> sys.stderr,  "call me with a filename - either an axis.ini file or an EMC2 tool table file"
        sys.exit(1)

    print '''<?xml version="1.0" encoding="UTF-8" ?>
<HeeksCAD_Document>'''

    for t in tool_table.entries:
        print t.heeks_tool
        
    print '</HeeksCAD_Document>\n'


    
