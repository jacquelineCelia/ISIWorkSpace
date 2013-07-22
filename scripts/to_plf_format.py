#!/usr/bin/python
import sys
import subprocess
import string
from optparse import OptionParser

def main():
    parser = OptionParser()
    parser.add_option("-f", dest = "infst", help = "input topologically sorted fst")
    parser.add_option("-o", dest = "output", help = "output file in giza format")
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.error("incorrect number of arguments, please type -h for more help.")

    tmpfile = options.infst + '.tmp'
    cmd = 'fstprint ' + options.infst + ' > ' + tmpfile
    subprocess.Popen(cmd, shell=True).wait()

if __name__ == "__main__":
    main()
