#ifndef _HMM_ALIGNER_H_
#define _HMM_ALIGNER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>

#include "LearningInfo.h"
#include "StringUtils.h"
#include "FstUtils.h"
#include "IAlignmentModel.h"
#include "IAlignmentSampler.h"
#include "MultinomialParams.h"

using namespace fst;
using namespace std;
using namespace MultinomialParams;

#define NULL_SRC_TOKEN_ID 1
#define NULL_SRC_TOKEN_STRING "__null__"
#define INITIAL_SRC_POS 0

class HmmAligner {
 public:
    HmmAligner(const string& bitextFilename,
               const string& outputFilenamePrefix,
               const LearningInfo& learningInfo,
               const string& tParamsFile, 
               const bool use_lattice);

    HmmAligner(const string& tParamsFile, \
               const string& aParamsFile, \
               const string& testFile,
               const float K = 1);

    void PersistParams(const string& outputFilename);
    void Train();
    void TrainWithLattices();
    void CreateTgtFsts();
    void CreateTgtFst(const vector<int> tgtTokens, VectorFst<FstUtils::LogQuadArc>&);

    string AlignSentWithLattice(vector<int> srcTokens, 
                     vector<int> tgtTokens, 
                     fst::VectorFst<FstUtils::LogQuadArc>& tgtFst);
    string AlignSent(vector<int> srcTokens, 
                     vector<int> tgtTokens);
    void AlignWithLattice(const string& alignmentsFilename);
    void Align(const string& alignmentsFilename);
    void CreatePerSentGrammarFsts();
    // normalizes the parameters such that \sum_t p(t|s) = 1 \forall s
    void NormalizeFractionalCounts();
    // creates a 1st order markov fst for each source sentence
    void CreateSrcFsts(vector< VectorFst< FstUtils::LogQuadArc > >& srcFsts);
    // creates an fst for each src sentence, which remembers the last visited src token
    void Create1stOrderSrcFst(const vector<int>& srcTokens, 
                              VectorFst<FstUtils::LogQuadArc>& srcFst);
    // create a grammar
    void CreatePerSentGrammarFst(vector<int> &srcTokens, 
                                 vector<int> &tgtTokens, 
                                 VectorFst<FstUtils::LogQuadArc>& perSentGrammarFst);
    // zero all parameters
    void ClearFractionalCounts();
    void LearnParameters();
    void BuildAlignmentFst(const VectorFst<FstUtils::LogQuadArc>& tgtFst, 
			               const VectorFst<FstUtils::LogQuadArc>& perSentGrammarFst,
			               const VectorFst<FstUtils::LogQuadArc>& srcFst, 
			               VectorFst<FstUtils::LogQuadArc>& alignmentFst);
    // load parameters
    void LoadInitTParams(const string& initTparams);
    void LoadInitAParams(const string& initAparams);
    void InitJustAParams();
    void NormalizePartialParams(unsigned int, vector<vector<double> >&);
    double LogSum(vector<double>& array);
    double FindLogMax(vector<double>& log_probs);

  private:
    string outputPrefix;
    vector<fst::VectorFst<FstUtils::LogQuadArc> > perSentGrammarFsts;

    // configurations
    LearningInfo learningInfo;

    // training data (src, tgt)
    vector< vector<int> > srcSents, tgtSents;
    vector<string> tgtLatticePaths;
    vector<fst::VectorFst<FstUtils::LogQuadArc> > tgtFsts;
  
    VocabEncoder vocabEncoder;
    /* aParams are used to describe p(a_i|a_{i-1}). 
       first key is the context (i.e. previous alignment a_{i-1}) 
       and nested key is the current alignment a_i. */
    ConditionalMultinomialParam<int> aFractionalCounts, aParams;
    ConditionalMultinomialParam<int> tFractionalCounts;
    bool use_lattice;
};

#endif
