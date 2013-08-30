#ifndef _MHIBM_H_
#define _MHIBM_H_

#include "FstUtils.h"
#include "MultinomialParams.h"

#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

#define NULL_SRC_TOKEN_ID 1
#define NULL_SRC_TOKEN_STRING "__null__"
#define INITIAL_SRC_POS 0

using namespace std;
using namespace fst;
using namespace MultinomialParams;

typedef VectorFst<LogArc> LogVectorFst;

class MHIBM {
 public:
    MHIBM(const string& fin, const string& basename); 
    void CreateTgtFsts(); 
    void CreateTgtFst(const vector<int>& sent, \
                      VectorFst<FstUtils::LogArc>& tgt_fst);
    float ComputeAlignmentScore(\
                const VectorFst<FstUtils::LogArc>& tgt_fst, \
                const vector<int>& tgt_sent, \
                const vector<int>& src_sent, \
                VectorFst<FstUtils::LogArc>& alignment, \
                vector<FstUtils::LogWeight>& alpha, \
                vector<FstUtils::LogWeight>& beta) ;
    void GatherPartialCounts(\
            const VectorFst<FstUtils::LogArc>& alignment, \
            const vector<FstUtils::LogWeight>& alpha, \
            const vector<FstUtils::LogWeight>& beta) ;
    int Train(int niter = 5000);
    vector<int> ProposeAPath(VectorFst<LogArc>& prior_fst);
    void InitializeParams();
    void UpdateParams();
    void NormalizeFractionalCounts();
    void DeepCopy(const ConditionalMultinomialParam<int>& original, \
                     ConditionalMultinomialParam<int>& duplicate); 
    void PersistParams(const string& outputFilename);
    void PrintSelectedPath(const string& output_filename); 
    vector<int> ReadSelectedPath(VectorFst<LogArc>& sampled);
    ~MHIBM() {};
 private:
    vector<vector<int> > tgt_sents_, src_sents_;
    vector<vector<int> > proposals_;
    vector<int> src_longest_path_lens_;
    vector<string> prior_fst_paths_; 
    vector<VectorFst<LogArc> > prior_fsts_;
    VocabEncoder vocabEncoder_;
    ConditionalMultinomialParam<int> aFractionalCounts_, aParams_;
    ConditionalMultinomialParam<int> tFractionalCounts_, tParams_;
    string basename_;
    vector<VectorFst<FstUtils::LogArc> > tgt_fsts_;
};

#endif
