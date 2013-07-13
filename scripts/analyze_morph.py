#!/usr/bin/python
import os
import sys
import string
import re
import tempfile, subprocess, hashlib, codecs
from optparse import OptionParser

__MAX_FILES = 500

def get_words(text):
   counter = 0
   words = list()
   ftext = open(text)
   line = ftext.readline()
   while line:
      tokens = line.strip().split()
      for t in tokens:
         if t not in words:
            words.append(t)
      counter += 1
      if counter % 1000 == 0:
         print '.'
      line = ftext.readline()
   ftext.close()
   return words

def hashed_path(parent, name, suffix):
   h = hashlib.md5(name).hexdigest()
   dir_num = int(h, 16) % __MAX_FILES
   if not parent.endswith(os.sep): parent += os.sep
   return parent + str(dir_num) + os.path.sep + h + suffix

def make_path_dirs(path):
   path_dir = os.path.dirname(path)
   if not os.path.exists(path_dir):
      os.makedirs(path_dir)

def get_word_lattice(words, morph_fst, outdir):
   for w in words:
      filename = hashed_path(outdir, w, ".mph")
      make_path_dirs(filename)
      w = w.replace('"', '\\"')
      cmd = 'echo "' + w + '" | flookup ' + morph_fst + ' > ' + filename 
      print cmd
      subprocess.Popen(cmd, shell=True).wait()

def main():
   parser = OptionParser()
   parser.add_option("-t", dest = "text", help = "text data in the African langauge")
   parser.add_option("-m", dest = "morph_fst", help = "location of the morph fst")
   parser.add_option("-d", dest = "outdir", help ="directory of where the output fsts go")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")

   words = get_words(options.text)
   get_word_lattice(words, options.morph_fst, options.outdir)

if __name__ == "__main__":
   main()
