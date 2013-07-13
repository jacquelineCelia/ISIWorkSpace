#!/usr/bin/python
import os
import sys
import string
import re
from optparse import OptionParser

def ParseData(fn, sens):
   fin = open(fn)
   line = fin.readline()
   while line:
      sens.append(line.strip())
      line = fin.readline()
   fin.close()

def MergeToBitex(eng_sens, fra_sens, fn_out):
   # sanity check
   if len(eng_sens) != len(fra_sens):
      print "number of alignments in english is different from that of foreign lang"
      print "# eng alignments: ", len(eng_sens), " # fra alignments: ", len(fra_sens) 
   else:
      fout = open(fn_out, 'w')
      for idx, fra_sen in enumerate(fra_sens):
         # need to remove [ ] and ( )
         fra_tokens = fra_sen.split()
         eng_tokens = eng_sens[idx].split()
         if len(fra_tokens) <= 80 and len(eng_tokens) <= 80 and len(fra_tokens) > 0 and len(eng_tokens) > 0:
               fout.write(fra_sen.strip() + ' ||| ' + eng_sens[idx].strip() + '\n')
         else:
            print "ignoring sentences that are too long", len(fra_tokens), ' ', len(eng_tokens)
            print fra_sen.strip(), '|||', eng_sens[idx].strip()
      fout.close()

def NormalizeText(s):
   s = re.sub('\[[^\[]*\]', '', s)
   s = re.sub('\([^\(]*\)', '', s)
   s = re.sub('\.\.*', ' . ', s)
   s = re.sub('  *', ' ', s)
   return s

def main():
   parser = OptionParser()
   parser.add_option("-e", dest = "eng", help = "english file")
   parser.add_option("-f", dest = "fra", help = "french file")
   parser.add_option("-o", dest = "out", help = "output file")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")

   eng_sens = list()
   fra_sens = list()
   
   print "parsing english data..."
   ParseData(options.eng, eng_sens)

   print "parsing foreign data..."
   ParseData(options.fra, fra_sens)

   print "merge into an output file..."
   MergeToBitex(eng_sens, fra_sens, options.out)

if __name__ == "__main__":
   main()
