#!/bin/bash

data_dir="$ROOT/swahili/data/"
root_dir="$MOSESROOT/mosesdecoder/"
work_dir=$1

$root_dir/scripts/training/train-model.perl -root-dir $1 -corpus $data_dir/$2 -f swa -e eng -alignment grow-diag-final-and -reordering msd-bidirectional-fe -lm 0:7:$ROOT/lm/train.eng.swa.7gram.arpa  -external-bin-dir $root_dir/tools/  -first-step=$3 -last-step=$4 >& $1.out

