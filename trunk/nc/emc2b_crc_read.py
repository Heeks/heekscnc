import iso_read as iso
import sys

# just use the iso reader

if __name__ == '__main__':
    parser = iso.ParserIso()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
