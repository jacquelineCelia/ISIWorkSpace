#!/usr/bin/python
import os
import sys
import string
import re
import tempfile, subprocess, hashlib, codecs
from optparse import OptionParser

__MAX_FILES = 500

def hashed_path(parent, name, suffix):
   h = hashlib.md5(name).hexdigest()
   dir_num = int(h, 16) % __MAX_FILES
   if not parent.endswith(os.sep): parent += os.sep
   return parent + str(dir_num) + os.path.sep + h + suffix

def LoadData(filename):
    words = list()
    fin = open(filename)
    line = fin.readline()
    while line:
        words.append(line.strip())    
        line = fin.readline()
    fin.close()
    return words

def GetMorphPath(sent_in_words, mdir):
    paths = list()
    for s in sent_in_words: 
        p = hashed_path(mdir, s, ".fst")
        paths.append(p)
    return paths

def FilterLongStrings(sents, tag, th):
    for idx, s in enumerate(sents):
        if len(s.split()) > th:
            tag[idx] = False

def FilterBigLattices(sents, tag, th):
    for idx, s in enumerate(sents):
        label_path = s.replace(".fst", ".txt.ilabel")
        counter = 0
        fin = open(label_path)
        line = fin.readline()
        while line:
            counter += 1
            line = fin.readline()
        fin.close()
        if counter > th:
            tag[idx] = False

def main():
    parser = OptionParser()
    parser.add_option("--word_f", dest = "word_f", help = "text data in the African langauge")
    parser.add_option("--word_e", dest = "word_e", help = "text data in the English langauge")
    parser.add_option("--morph", dest = "morph", help = "morph data of the African language")
    parser.add_option("--sent_class_morph_dir", dest = "class_dir", help = "directories that contain sentence morphs")
    parser.add_option("--sent_morph_dir", dest = "mdir", help = "directories that contain sentence morphs")
    parser.add_option("--prefix", dest = "prefix", help = "prefix for the output file")
    parser.add_option("--threshold", dest = "th", help = "threshold for the length", type = int, default = 250) 

    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.error("incorrect number of arguments, please type -h for more help.")

    eng_words = LoadData(options.word_e)
    fra_words = LoadData(options.word_f)
    fra_morphs = LoadData(options.morph)
    fra_sent_morphs = GetMorphPath(fra_words, options.mdir)
    fra_class_sent_morphs = GetMorphPath(fra_words, options.class_dir)

    if len(eng_words) != len(fra_words):
        print "# of english sents != # of french sents"
    elif len(eng_words) != len(fra_morphs):
        print "# of english sents != # of french morpheme analyses"
    else:
        tag = [True for i in range(len(eng_words))]
        FilterLongStrings(eng_words, tag, options.th)
        FilterLongStrings(fra_words, tag, options.th)
        FilterLongStrings(fra_morphs, tag, options.th)
        FilterBigLattices(fra_sent_morphs, tag, options.th)
        FilterBigLattices(fra_class_sent_morphs, tag, options.th)
        
        output_eng_words = options.prefix + ".eng"
        output_fra_words = options.prefix + ".kin"
        output_fra_morphs = options.prefix + ".morph.kin" 
        output_fra_lattices = options.prefix + ".lattice.kin"
        output_fra_class_lattices = options.prefix + ".class.lattice.kin"

        foutput_eng_words = open(output_eng_words, 'w')
        foutput_fra_words = open(output_fra_words, 'w')
        foutput_fra_morphs = open(output_fra_morphs, 'w')
        foutput_fra_lattices = open(output_fra_lattices, 'w')
        foutput_fra_class_lattices = open(output_fra_class_lattices, 'w')

        for idx, t in enumerate(tag):
            if t:
                foutput_eng_words.write(eng_words[idx] + '\n')
                foutput_fra_words.write(fra_words[idx] + '\n')
                foutput_fra_morphs.write(fra_morphs[idx] + '\n')
                foutput_fra_lattices.write(fra_sent_morphs[idx] + '\n')
                foutput_fra_class_lattices.write(fra_class_sent_morphs[idx] + '\n')

        foutput_eng_words.close()
        foutput_fra_words.close()
        foutput_fra_morphs.close()
        foutput_fra_lattices.close()
        foutput_fra_class_lattices.close()

if __name__ == "__main__":
    main()

