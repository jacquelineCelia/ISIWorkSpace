#!/usr/bin/python
from optparse import OptionParser

def main():
   parser = OptionParser()
   parser.add_option("-i", dest = "infile", help = "input align file")
   parser.add_option("-o", dest = "outfile", help = "output file for training in eng-kin direction")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")
   
   fin = open(options.infile)
   fout = open(options.outfile, 'w')
   line = fin.readline()

   while line:
       data = fin.readline()
       fout.write(data)
       line = fin.readline()

   fin.close()
   fout.close()

if __name__ == "__main__":
   main()
