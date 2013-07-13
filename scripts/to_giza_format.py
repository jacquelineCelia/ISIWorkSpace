#!/usr/bin/python
from optparse import OptionParser

def load_text_file(fn_source):
   source = list()
   fin = open(fn_source)
   line = fin.readline()
   while line:
      tokens = line.strip().split()
      source.append(tokens)
      line = fin.readline()
   fin.close()
   return source

def parse_alignments(fn_align):
   aligns = list()
   fin = open(fn_align)
   line = fin.readline()
   while line:
      mapping = dict()
      aligns.append(mapping)
      tokens = line.strip().split()
      for t in tokens:
         pair = t.split('-')
         if int(pair[0]) not in mapping:
            mapping[int(pair[0])] = list()
         mapping[int(pair[0])].append(int(pair[1]))
      line = fin.readline()
   fin.close()
   return aligns

def get_null_mapping(a, len_t):
   marks = list()
   null = list()
   for t in a:
      for m in a[t]:
         marks.append(m)
   
   for i in range(len_t):
      if i not in marks:
         null.append(i)
   return null

def merge_to_giza(source, target, aligns, fn_out):
   fout = open(fn_out, 'w')
   if (len(source) != len(target)):
      print "number of entries in source != that of target"
   else:
      for i in range(len(source)):
         s = source[i]
         t = target[i]
         a = aligns[i]
         null = get_null_mapping(a, len(t))
         fout.write("# Sentence pair (" + str(i + 1) + ") source length " + str(len(s)) + " target length " + str(len(t)) + " alignment score : 0.0\n")
         sen = ' '.join(t)
         fout.write(sen + '\n')
         fout.write('NULL ({ ')
         for j in null:
            fout.write(str(j + 1) + ' ')
         fout.write('})')
         for s_id in range(len(s)):
            fout.write(' ' + s[s_id] + ' ({ ')
            if s_id not in a:
               fout.write('})')
            else:
               for j in a[s_id]:
                  fout.write(str(j + 1) + ' ')
               fout.write('})')
         fout.write('\n')
   fout.close()

def main():
   parser = OptionParser()
   parser.add_option("-s", dest = "source", help = "source text file")
   parser.add_option("-t", dest = "target", help = "target file")
   parser.add_option("-a", dest = "align", help = "alignment file, source-target (un-unormalized)")
   parser.add_option("-o", dest = "output", help = "output file in giza format")
   (options, args) = parser.parse_args()
   if len(args) != 0:
      parser.error("incorrect number of arguments, please type -h for more help.")

   source = load_text_file(options.source)
   target = load_text_file(options.target)
   aligns = parse_alignments(options.align)
   merge_to_giza(source, target, aligns, options.output)

if __name__ == "__main__":
   main()
