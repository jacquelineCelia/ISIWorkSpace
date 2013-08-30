CC=/usr/bin/g++
SINGLE=-c
#BEFORE=-x c++ -std=c++11
LIBS=-lfst -ldl
OPT=-O3
DEBUG=-g -ggdb

all: train-hmmAligner

# specific to the word alignment hmm model
train-hmmAligner: train-hmmAligner.o
	$(CC) train-hmmAligner.o HmmAligner.o IbmModel1.o FstUtils.o $(LIBS) -o train-hmmAligner

train-hmmAligner.o: FstUtils.o HmmAligner.o IbmModel1.o train-hmmAligner.cc IbmModel1.cc LearningInfo.h StringUtils.h
	$(CC) $(BEFORE) $(SINGLE) train-hmmAligner.cc $(OPT)

HmmAligner.o: FstUtils.o HmmAligner.cc HmmAligner.h LearningInfo.h StringUtils.h MultinomialParams.h
	$(CC) $(BEFORE) $(SINGLE) HmmAligner.cc $(OPT)

FstUtils.o: FstUtils.cc FstUtils.h
	$(CC) $(BEFORE) $(SINGLE) FstUtils.cc $(OPT)

IbmModel1.o: FstUtils.cc IbmModel1.cc IbmModel1.h LearningInfo.h StringUtils.h IAlignmentModel.h MultinomialParams.h VocabEncoder.h 
	$(CC) $(BEFORE) $(SINGLE) IbmModel1.cc $(OPT)

clean:
	rm -rf FstUtils.o IbmModel1.o HmmAligner.o train-hmmAligner.o train-hmmAligner
