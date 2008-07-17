import sys

def redirfn():
    fsock = open('error.log', 'w')
    sys.stderr = fsock

def redirflushfn():
    sys.stderr.flush()
