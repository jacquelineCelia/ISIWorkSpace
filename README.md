Experiment material of the summer.

1). Training alignment model

There are three ways to train IBM model 1 and the HMM model with lattices. 

Basic - Learn P(f,G|e) with each path in G being equal.
Weighted - Learn P(f,G|e), but paths in G are weighted unequally.
MH - Learn P(e,f|G) using Metropolis Hastings   

===========================
Basic
===========================
cd $ROOT/basic
./train-model1 bitext output_prefix use_lattice
e.g., ./train-model1 lattice.a2.train.norm.bitxt result/lattice_a2/ibm 1

./train-hmmAligner 1 bitextFilename outputFilePathPrefix tParameters_init_file use_lattice
e.g., ./train-hmmAligner 1 lattice.a2.train.norm.bitxt result/lattice_a2/hmm result/lattice_a2/ibm.param.final 1

===========================
Weighted
===========================
cd $ROOT/weight_times
./train-model1 bitext output_prefix use_lattice
e.g., ./train-model1 lattice.a2.train.norm.bitxt result/lattice_a2/ibm 1

./train-hmmAligner 1 bitextFilename outputFilePathPrefix tParameters_init_file use_lattice
e.g., ./train-hmmAligner 1 lattice.a2.train.norm.bitxt result/lattice_a2/hmm result/lattice_a2/ibm.param.final 1

===========================
MH
===========================
cd $ROOT/MH 
./train-mh 0[ibm training] bitext basename
e.g., ./train-mh 0 lattice.a2.train.norm.bitxt result/lattice_a2/ibm 

./train-mh 1[hmm training] bitext basename tparams
e.g., ./train-mh 1 lattice.a2.train.norm.bitxt result/lattice_a2/hmm  result/lattice_a2/ibm.1000.t 

2. Generate parallel morpheme-to-sentence corpus

Take the MH sampling result as an example.

cd $ROOT/scripts
./get_retrain_data.py -i $ROOT/result/lattice_a2/hmm.1000.align -o $ROOT/data/lattice.a2.mh.train.swa
cat $ROOT/data/clean/train.swa >> $ROOT/data/lattice.a2.mh.train.swa
cp $ROOT/data/clean.train.eng $ROOT/data/lattice.a2.mh.train.eng
cat $ROOT/data/clean.train.eng >> $ROOT/data/lattice.a2.mh.train.eng

3. Train and test the translation model 
cd moses

Change train.sh, binarize.sh dev.sh and test.sh such that it points to your moses binaries.

./train.sh lattice_a2_mh lattice.a2.mh.train 1 9 ; ./binarize.sh lattice_a2_mh ; ./dev.sh lattice_a2_mh a2.base ; ./test.sh lattice_a2_mh a2.base;

After decoding is done, do 
tail lattice_a2_mh/translation.out

and you will get the bleu score.

4. Baselines to compare against 

Word-based: 22.20
UnMorph: 23.37
Lattice-basic: 22.74 (a2)
Lattice-weighted: 23.47
Lattice-mh: to be updated


