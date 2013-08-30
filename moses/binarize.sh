#!/bin/bash

root_dir="$MOSESROOT/mosesdecoder/"
work_dir=$1

mkdir $work_dir/binarised-model

$root_dir/bin/processPhraseTable -ttable 0 0 $work_dir/model/phrase-table.gz -nscores 5 -out $work_dir/binarised-model/phrase-table >& $work_dir/binarised-pt.out
$root_dir/bin/processLexicalTable -in $work_dir/model/reordering-table.wbe-msd-bidirectional-fe.gz -out $work_dir/binarised-model/reordering-table >& $work_dir/binarised-lex.out

sed "s/1best_lattice/$work_dir/" moses.standard.ini > $work_dir/binarised-model/moses.ini 
