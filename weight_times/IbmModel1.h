#ifndef _IBM_MODEL_1_H_
#define _IBM_MODEL_1_H_

#include <iostream>
#include <fstream>
#include <math.h>

#include "LearningInfo.h"
#include "StringUtils.h"
#include "FstUtils.h"
#include "IAlignmentModel.h"
#include "MultinomialParams.h"

using namespace fst;
using namespace std;

#define NULL_SRC_TOKEN_ID 1
#define NULL_SRC_TOKEN_STRING "__null__"
#define INITIAL_SRC_POS 0 

class IbmModel1 {
 public:
  // bitextFilename is formatted as:
  // source sentence ||| target sentence
  IbmModel1(const string& bitextFilename, 
	        const string& outputFilenamePrefix, 
	        const LearningInfo& learningInfo,
            const bool use_lattice);

  void ConstructWithLattice();
  void Construct();

  void PersistParams(const string& outputFilename);
  
  void InitParams();
  void TrainWithLattices();
  void Train();

  void CreateTgtFsts();

  // virtual void Align();
  // void Align(const string &alignmentsFilename);

  // normalizes the parameters such that \sum_t p(t|s) = 1 \forall s
  void NormalizeParams();
  void CreatePerSentGrammarFsts(vector<VectorFst<FstUtils::LogArc> >& perSentGrammarFsts);
  
  // zero all parameters
  void ClearParams();
  void LearnParameters();

 private:
  string bitextFilename, outputPrefix;
  vector<VectorFst<FstUtils::LogArc> > tgtFsts;
  LearningInfo learningInfo;
  vector<vector<int> > srcSents, tgtSents;
  vector<string> tgtLatticePaths;
  MultinomialParams::ConditionalMultinomialParam<int> params;
  VocabEncoder vocabEncoder;
};

#endif
