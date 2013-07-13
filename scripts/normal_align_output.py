#!/usr/bin/python
import os
import sys
import string
import re
from optparse import OptionParser

def swap(tokens):
   if len(tokens) != 2:
      print "there should be only two tokens in the array!"
   else:
      a = tokens[0]
      b = tokens[1]
      tokens = [b, a]
   return tokens

def Normalize(fn_in, fn_out):
   fin = open(fn_in)
   fout = open(fn_out, 'w')
   line = fin.readline()
   while line:
      tokens = line.split()
      newtokens = []
      for t in tokens:
         subtokens = t.split('-')
         newtokens.append('-'.join(swap(subtokens)))
      newline = ' '.join(newtokens) + '\n'
      fout.write(newline)
      line = fin.readline()
   fin.close()
   fout.close()

def main():
   parser = OptionParser()
   parser.add_option("-i", dest = "input", help = "input file")
   parser.add_option("-o", dest = "output", help = "output file")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")

   Normalize(options.input, options.output)

if __name__ == "__main__":
   main()
