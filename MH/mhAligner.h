#ifndef __MH_ALIGNER__
#define __MH_ALIGNER__

#include "FstUtils.h"
#include "MultinomialParams.h"
#include "prob.h"

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

class MHAligner {
 public:
     MHAligner(const string& fin, const string& basename) {
         basename_ = basename;
         vocabEncoder_.useUnk = false; 
         vocabEncoder_.ReadParallelCorpus(fin, \
                                        tgt_sents_, \
                                        prior_fst_paths_);
         assert(tgt_sents_.size() > 0 && \
             tgt_sents_.size() == prior_fst_paths_.size());
         FstUtils::LoadLatticeFsts(prior_fst_paths_, src_sents_, prior_fsts_, vocabEncoder_);
         vector<VectorFst<LogArc> >::iterator piter = prior_fsts_.begin();
         for (; piter != prior_fsts_.end(); ++piter) {
             int len = FstUtils::GetLongestPathLen(*piter);
             src_longest_path_lens_.push_back(len);
         }
         vector<int> def(1, -1);
         proposals_.resize(prior_fsts_.size(), def);
     }
     virtual void CreateTgtFsts() = 0;
     virtual int Train(int niter = 5000) = 0;
     vector<int> ProposeAPath(VectorFst<LogArc>& prior_fst) {
         fst::VectorFst<fst::LogArc> sampled;
         std::random_device rd;
         std::mt19937 engine(rd());
         int seed = prob::randint(engine, -INT_MAX, INT_MAX);
         fst::LogProbArcSelector<fst::LogArc> selector(seed);
         fst::RandGenOptions<fst::LogProbArcSelector<fst::LogArc> > options(selector);
         fst::RandGen(prior_fst, &sampled, options);
         return ReadSelectedPath(sampled);
     }
     void InitializeParams() {
         assert(tgt_sents_.size() == src_sents_.size());
         assert(src_sents_.size() == src_longest_path_lens_.size());
         int n = tgt_sents_.size();
         for (int i = 0; i < n; ++i) {
             vector<int> tgt_tokens = tgt_sents_[i];
             vector<int> src_tokens = src_sents_[i];
             for (size_t s_pos = 0; s_pos < src_tokens.size(); ++s_pos) {
                 int src_token = src_tokens[s_pos];
                 for (size_t t_pos = 0; t_pos < tgt_tokens.size(); ++t_pos) {
                     int tgt_token = tgt_tokens[t_pos];
                     if (tFractionalCounts_[src_token][tgt_token] == 0) {
                         tFractionalCounts_[src_token][tgt_token] = 1; 
                     }
                     else { 
                         tFractionalCounts_[src_token][tgt_token] += 1; 
                     }
                 }
             } 
             for (int s_pos = 1; s_pos < src_longest_path_lens_[i]; ++s_pos) {
                 for (int k = 1 ; k < src_longest_path_lens_[i]; ++k) {
                     if (aFractionalCounts_[s_pos].count(k) == 0) {
                         aFractionalCounts_[s_pos][k] = 1;
                     }
                 }
             }
             for (int k = 0; i < src_longest_path_lens_[i]; ++k) {
                 if (aFractionalCounts_[0].count(k) == 0) {
                     aFractionalCounts_[0][k] = 1;
                 }
             }
         }
         UpdateParams();
         PersistParams(basename_ + ".init");
     }
     void UpdateParams() {
         NormalizeFractionalCounts(); 
         DeepCopy(tFractionalCounts_, tParams_);
         DeepCopy(aFractionalCounts_, aParams_);
         ClearParams(tFractionalCounts_);
         ClearParams(aFractionalCounts_);
     }
     void NormalizeFractionalCounts() {
         NormalizeParams(aFractionalCounts_);
         NormalizeParams(tFractionalCounts_);
     }
     void DeepCopy(const ConditionalMultinomialParam<int>& original, \
                   ConditionalMultinomialParam<int>& duplicate) {
         ClearParams(duplicate);
         for (map<int, MultinomialParam>::const_iterator contextIter = original.params.begin(); \
                 contextIter != original.params.end(); contextIter ++) {
             for (MultinomialParam::const_iterator multIter = \
                     contextIter -> second.begin(); \
                     multIter != contextIter -> second.end(); multIter ++) {
                 duplicate[contextIter -> first][multIter -> first] = multIter -> second;
             }
         }
     }
     void PersistParams(const string& outputFilename) {
         string translationFilename = outputFilename + string(".t");
         MultinomialParams::PersistParams(translationFilename, tFractionalCounts_, vocabEncoder_, true, true);
         string transitionFilename = outputFilename + string(".a");
         MultinomialParams::PersistParams(transitionFilename, aFractionalCounts_, vocabEncoder_, false, false);
     }
     vector<int> ReadSelectedPath(VectorFst<LogArc>& sampled) {
        vector<int> src_sent;
        StateIterator<VectorFst<LogArc> > siter(sampled);
        while (!siter.Done()) {
            ArcIterator<LogVectorFst> aiter(sampled, siter.Value());
            src_sent.push_back(aiter.Value().ilabel);
            aiter.Next();
            if (!aiter.Done()) {
                cerr << "there are more than one arc leaving a state!\n";
            }
            siter.Next();
        }
        return src_sent;
     }
     ~MHAligner() {};
 protected: 
     vector<vector<int> > tgt_sents_, src_sents_;
     vector<vector<int> > proposals_;
     vector<int> src_longest_path_lens_;
     vector<string> prior_fst_paths_; 
     vector<VectorFst<LogArc> > prior_fsts_;
     VocabEncoder vocabEncoder_;
     ConditionalMultinomialParam<int> aFractionalCounts_, aParams_;
     ConditionalMultinomialParam<int> tFractionalCounts_, tParams_;
     string basename_;
};


class MHIBM : public MHAligner {
    public:
        MHIBM(const string& fin, const string& basename) : \
            MHAligner(fin, basename) {};
        int Train(int niter) {
            std::random_device rd;
            std::mt19937 engine(rd());
            CreateTgtFsts();
            InitializeParams();
            for (int n = 0; n < niter; ++n) { 
                float corpus_likelihood = 0;
                if (!n) {
                    for (int i = 0; i < prior_fsts_.size(); ++i) {
                        vector<int > new_src = ProposeAPath(prior_fsts_[i]);
                        VectorFst<FstUtils::LogArc> alignment;
                        vector<FstUtils::LogWeight> alpha, beta;
                        corpus_likelihood += \
                            ComputeAlignmentScore(tgt_fsts_[i], tgt_sents_[i], \
                                new_src, alignment, alpha, beta);
                        GatherPartialCounts(alignment, alpha, beta);
                        proposals_[i] = new_src;
                    } 
                    UpdateParams();
                }
                else {
                    for (int i = 0; i < prior_fsts_.size(); ++i) {
                        vector<int > new_src = ProposeAPath(prior_fsts_[i]);
                        VectorFst<FstUtils::LogArc> new_alignment;
                        vector<FstUtils::LogWeight> new_alpha, new_beta;
                        float new_src_score = ComputeAlignmentScore(tgt_fsts_[i], tgt_sents_[i], \
                                new_src, new_alignment, new_alpha, new_beta);
                        VectorFst<FstUtils::LogArc> old_alignment;
                        vector<FstUtils::LogWeight> old_alpha, old_beta;
                        float old_src_score = ComputeAlignmentScore(tgt_fsts_[i], tgt_sents_[i], \
                                proposals_[i], old_alignment, old_alpha, old_beta);
                        if (new_src_score >= old_src_score) {
                            GatherPartialCounts(new_alignment, new_alpha, new_beta);
                            proposals_[i] = new_src;
                            corpus_likelihood += new_src_score;
                        }
                        else {
                            if (prob::random(engine) <= new_src_score / old_src_score) {
                                GatherPartialCounts(new_alignment, new_alpha, new_beta);
                                proposals_[i] = new_src;
                                corpus_likelihood += new_src_score;
                            } 
                            else {
                                GatherPartialCounts(old_alignment, old_alpha, old_beta);
                                corpus_likelihood += old_src_score;
                            }
                        }
                    } 
                    UpdateParams();
                }
            }
            return 0;
        }
        void CreateTgtFsts() {
            vector<vector<int> >::iterator siter = tgt_sents_.begin();
            for (; siter != tgt_sents_.end(); ++siter) {
                VectorFst<FstUtils::LogArc> tgt_fst;
                CreateTgtFst(*siter, tgt_fst);
                tgt_fsts_.push_back(tgt_fst);
            }
        }
        void CreateTgtFst(const vector<int>& sent, VectorFst<FstUtils::LogArc>& tgt_fst) {
            assert(tgt_fst.NumStates() == 0);
            for (int i = 0; i < sent.size(); ++i) {
                int currentState = tgt_fst.AddState();
                assert(currentState == i);
                tgt_fst.AddArc(currentState, \
                        FstUtils::LogArc(sent[i], sent[i], 0, currentState + 1));
            }
            int final_state = tgt_fst.AddState();
            assert(final_state == (int) sent.size());
            tgt_fst.SetStart(0);
            tgt_fst.SetFinal(final_state, FstUtils::LogWeight::One());
            ArcSort(&tgt_fst, ILabelCompare<FstUtils::LogArc>());
        }
        float ComputeAlignmentScore(\
                const VectorFst<FstUtils::LogArc>& tgt_fst, \
                const vector<int>& tgt_sent, \
                const vector<int>& src_sent, \
                VectorFst<FstUtils::LogArc>& alignment, \
                vector<FstUtils::LogWeight>& alpha, \
                vector<FstUtils::LogWeight>& beta) {
            VectorFst<FstUtils::LogArc> grammar;
            set<int> tgt_tokens(tgt_sent.begin(), tgt_sent.end());
            int state_id = grammar.AddState();
            assert(state_id == 0);
            vector<int>::const_iterator siter = src_sent.begin();
            for (; siter != src_sent.end(); ++siter) {
                set<int>::const_iterator titer = tgt_tokens.begin();
                for (; titer != tgt_tokens.end(); ++titer) {
                    grammar.AddArc(state_id, \
                            FstUtils::LogArc(*titer, *siter, \
                                tParams_[*siter][*titer], state_id));
                }
            }
            grammar.SetStart(state_id);
            grammar.SetFinal(state_id, FstUtils::LogWeight::One());
            ArcSort(&grammar, ILabelCompare<FstUtils::LogArc>());
            Compose(tgt_fst, grammar, &alignment);
            ShortestDistance(alignment, &alpha, false);
            ShortestDistance(alignment, &beta, true);
            return beta[alignment.Start()].Value();
        }
        void GatherPartialCounts(const VectorFst<FstUtils::LogArc>& alignment, \
                                 const vector<FstUtils::LogWeight>& alpha, \
                                 const vector<FstUtils::LogWeight>& beta) {
            float total_prob = beta[alignment.Start()].Value();
            StateIterator<VectorFst<FstUtils::LogArc> > siter(alignment);
            while (!siter.Done()) {
                ArcIterator<VectorFst<FstUtils::LogArc> > aiter(alignment, siter.Value());
                while (!aiter.Done()) {
                    int src_token = aiter.Value().olabel;
                    int tgt_token = aiter.Value().ilabel;
                    int from = siter.Value();
                    int to = aiter.Value().nextstate;
                    FstUtils::LogWeight arc_weight = aiter.Value().weight;
                    FstUtils::LogWeight total_prob_through_arc = \
                        Times(Times(alpha[from], arc_weight), beta[to]);
                    FstUtils::LogWeight normalized_weight = \
                        total_prob_through_arc.Value() - total_prob;
                    tFractionalCounts_[src_token][tgt_token] = \
                        Plus(\
                            FstUtils::LogWeight(\
                                tFractionalCounts_[src_token][tgt_token]), \
                            normalized_weight).Value();
                }
            }
        }
        ~MHIBM() {};
    private:
        vector<VectorFst<FstUtils::LogArc> > tgt_fsts_;
};

class MHHMM : public MHAligner {
    public:
        MHHMM(const string& fin, const string& basename) : MHAligner(fin, basename) {};
        int Train(int niter) {
            std::random_device rd;
            std::mt19937 engine(rd());
            CreateTgtFsts();
            InitializeParams();
            for (int n = 0; n < niter; ++n) {
                float corpus_likelihood = 0;
                if (!n) {
                    for (int i = 0; i < prior_fsts_.size(); ++i) {
                        vector<int> new_src = ProposeAPath(prior_fsts_[i]);
                        VectorFst<FstUtils::LogQuadArc> alignment;
                        vector<FstUtils::LogQuadWeight> alpha, beta;
                        corpus_likelihood += ComputeAlignmentScore(\
                                tgt_fsts_[i], tgt_sents_[i], new_src, \
                                alignment, alpha, beta);
                        GatherPartialCounts(alignment, alpha, beta);
                        proposals_[i] = new_src;
                    }
                } 
                else {
                    for (int i = 0; i < prior_fsts_.size(); ++i) {
                        vector<int> new_src = ProposeAPath(prior_fsts_[i]);
                        VectorFst<FstUtils::LogQuadArc> new_alignment;
                        vector<FstUtils::LogQuadWeight> new_alpha, new_beta;
                        float new_src_score = ComputeAlignmentScore(\
                                tgt_fsts_[i], tgt_sents_[i], new_src, \
                                new_alignment, new_alpha, new_beta); 
                        VectorFst<FstUtils::LogQuadArc> old_alignment;
                        vector<FstUtils::LogQuadWeight> old_alpha, old_beta;
                        float old_src_score = ComputeAlignmentScore(\
                                tgt_fsts_[i], tgt_sents_[i], proposals_[i], \
                                old_alignment, old_alpha, old_beta); 
                        if (new_src_score > old_src_score) {
                            GatherPartialCounts(new_alignment, new_alpha, new_beta);
                            proposals_[i] = new_src;
                            corpus_likelihood += new_src_score;
                        }
                        else {
                            if (prob::random(engine) <= new_src_score / old_src_score) {
                                GatherPartialCounts(new_alignment, new_alpha, new_beta);
                                proposals_[i] = new_src;
                                corpus_likelihood += new_src_score;
                            }
                            else {
                                GatherPartialCounts(old_alignment, old_alpha, old_beta);
                                corpus_likelihood += old_src_score;
                            }
                        }
                    }
                }
                UpdateParams();
            }
            return 0;
        }
        void CreateTgtFsts() {
            vector<vector<int> >::iterator siter = tgt_sents_.begin();
            for (; siter != tgt_sents_.end(); ++siter) {
                VectorFst<FstUtils::LogQuadArc> tgt_fst; 
                CreateTgtFst(*siter, tgt_fst);
                tgt_fsts_.push_back(tgt_fst);
            }
        }
        void CreateTgtFst(const vector<int>& sent, VectorFst<FstUtils::LogQuadArc>& tgt_fst) {
            assert(tgt_fst.NumStates() == 0);
            for (int i = 0; i < sent.size(); ++i) {
                int currentState = tgt_fst.AddState();
                assert(currentState == i);
                tgt_fst.AddArc(currentState, \
                FstUtils::LogQuadArc(sent[i], sent[i], \
                    FstUtils::EncodeQuad(currentState + 1, 0, 0, 0), currentState + 1));
            }
            int final_state = tgt_fst.AddState();
            assert(final_state == (int) sent.size());
            tgt_fst.SetStart(0);
            tgt_fst.SetFinal(final_state, FstUtils::LogQuadWeight::One());
            ArcSort(&tgt_fst, ILabelCompare<FstUtils::LogQuadArc>());
        }
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
        ~MHHMM() {};
    private:
        vector<VectorFst<FstUtils::LogQuadArc> > tgt_fsts_;
};

#endif
