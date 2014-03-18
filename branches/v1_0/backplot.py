import sys
from nc.hxml_writer import HxmlWriter

if len(sys.argv)>2:
    reader = sys.argv[1]
    nc_file = sys.argv[2]
    
    machine_module = __import__('nc.' + reader, fromlist = ['dummy'])
        
    parser = machine_module.Parser(HxmlWriter())

    parser.Parse(nc_file)
