#ifndef _MHHMM_H_
#define _MHHMM_H_

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

class MHHMM {
 public:
     MHHMM(const string& fin, const string& basename ="");
     int Train(int niter = 5000, const string& tparams="");
     vector<int> ProposeAPath(VectorFst<LogArc>& prior_fst);
     void InitializeParams();
     void LoadTParams(const string& tparams);
     void UpdateParams();
     void NormalizeFractionalCounts();
     void DeepCopy(const ConditionalMultinomialParam<int>& original, \
            ConditionalMultinomialParam<int>& duplicate);
     vector<int> ReadSelectedPath(VectorFst<LogArc>& sampled);
     void PersistParams(const string& output_filename);
     void PrintSelectedPath(const string& output_filename); 
     void CreateTgtFsts();
     void CreateTgtFst(const vector<int>& sent, VectorFst<FstUtils::LogQuadArc>& tgt_fst);
     float ComputeAlignmentScore(\
            const VectorFst<FstUtils::LogQuadArc>& tgt_fst, \
            const vector<int>& tgt_sent, \
            const vector<int>& src_sent, \
            VectorFst<FstUtils::LogQuadArc>&, \
            vector<FstUtils::LogQuadWeight>&, \
            vector<FstUtils::LogQuadWeight>&);
     void GatherPartialCounts(const VectorFst<FstUtils::LogQuadArc>&, \
                              const vector<FstUtils::LogQuadWeight>&, \
                              const vector<FstUtils::LogQuadWeight>&);
     void CreateTranslationGrammar(const vector<int>&, \
                const vector<int>&, VectorFst<FstUtils::LogQuadArc>&);
     void CreateTransitionFst(const vector<int>&, \
                              VectorFst<FstUtils::LogQuadArc>&);
     void NormalizePartialParams(unsigned int n, \
                                 vector<vector<double> >& prob);
     double LogSum(vector<double>& array);
     double FindLogMax(vector<double>& log_probs);
     ~MHHMM() {};
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
     vector<VectorFst<FstUtils::LogQuadArc> > tgt_fsts_;
};

#endif
