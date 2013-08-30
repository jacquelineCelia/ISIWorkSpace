#include "mhhmm.h"
#include "prob.h"

MHHMM::MHHMM(const string& fin, const string& basename) {
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

void MHHMM::CreateTgtFsts() {
    vector<vector<int> >::iterator siter = tgt_sents_.begin();
    for (; siter != tgt_sents_.end(); ++siter) {
        VectorFst<FstUtils::LogQuadArc> tgt_fst; 
        CreateTgtFst(*siter, tgt_fst);
        tgt_fsts_.push_back(tgt_fst);
    }
}

int MHHMM::Train(int niter, const string& tparams) {
    std::random_device rd;
    std::mt19937 engine(rd());
    CreateTgtFsts();
    InitializeParams();
    if (tparams != "") {
        cerr << "loading T params..." << endl;
        LoadTParams(tparams);
    }
    cerr << "Training..." << endl;
    for (int n = 0; n < niter; ++n) {
        float corpus_likelihood = 0;
        if (!n) {
            for (int i = 0; i < prior_fsts_.size(); ++i) {
                if (i % 100 == 0) {
                    cerr << "." ;
                }
                vector<int> new_src = ProposeAPath(prior_fsts_[i]);
                VectorFst<FstUtils::LogQuadArc> alignment;
                vector<FstUtils::LogQuadWeight> alpha, beta;
                corpus_likelihood += ComputeAlignmentScore(\
                        tgt_fsts_[i], tgt_sents_[i], new_src, \
                        alignment, alpha, beta);
                GatherPartialCounts(alignment, alpha, beta);
                proposals_[i] = new_src;
            }
            cerr << endl;
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
                if (new_src_score <= old_src_score) { // new_src_score and old_src_score are in log space
                    GatherPartialCounts(new_alignment, new_alpha, new_beta);
                    proposals_[i] = new_src;
                    corpus_likelihood += new_src_score;
                }
                else {
                    // new_src_score and old_src_score are in log space
                    if (prob::random(engine) <= exp(old_src_score - new_src_score)) {
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
        cerr << "Iter " << n << ": CorpusLikelihood = " << corpus_likelihood << endl;
        UpdateParams();
        if (n % 100 == 0 && n) {
            stringstream iter;
            iter << n;
            PersistParams(basename_ + "." + iter.str());
            PrintSelectedPath(basename_ + "." + iter.str() + ".align"); 
        }
    }
    return 0;
}

void MHHMM::PrintSelectedPath(const string& filename) {
    ofstream fout(filename.c_str());
    for (size_t i = 0; i < proposals_.size(); ++i) {
        vector<int> f = proposals_[i];
        string path = "";
        for (size_t j = 0; j < f.size(); ++j) {
            string morph = vocabEncoder_.Decode(f[j]);
            if (morph != NULL_SRC_TOKEN_STRING) {
                path += morph + " ";
            }
        }
        path += "\n";
        fout << path;
    }
    fout.close();
}

vector<int> MHHMM::ProposeAPath(VectorFst<LogArc>& prior_fst) {
    fst::VectorFst<fst::LogArc> sampled;
    std::random_device rd;
    std::mt19937 engine(rd());
    int seed = prob::randint(engine, -INT_MAX, INT_MAX);
    fst::LogProbArcSelector<fst::LogArc> selector(seed);
    fst::RandGenOptions<fst::LogProbArcSelector<fst::LogArc> > options(selector);
    fst::RandGen(prior_fst, &sampled, options);
    return ReadSelectedPath(sampled);
}

void MHHMM::LoadTParams(const string& tparams) {
    ifstream infile(tparams.c_str());
    string src_token, tgt_token; 
    double t;
    while (infile >> tgt_token >> src_token >> t) {
        int src = vocabEncoder_.Encode(src_token, false);
        int tgt = vocabEncoder_.Encode(tgt_token, false); 
        tParams_[src][tgt] = t;
    } 
    infile.close();
}

void MHHMM::InitializeParams() {
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
                    tFractionalCounts_[src_token][tgt_token] = 1; 
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
        for (int k = 0; k < src_longest_path_lens_[i]; ++k) {
            if (aFractionalCounts_[0].count(k) == 0) {
                aFractionalCounts_[0][k] = 1;
            }
        }
    }
    UpdateParams();
    PersistParams(basename_ + ".init");
}

void MHHMM::UpdateParams() {
    NormalizeFractionalCounts(); 
    DeepCopy(tFractionalCounts_, tParams_);
    DeepCopy(aFractionalCounts_, aParams_);
    ClearParams(tFractionalCounts_);
    ClearParams(aFractionalCounts_);
}

void MHHMM::NormalizeFractionalCounts() {
    NormalizeParams(aFractionalCounts_);
    NormalizeParams(tFractionalCounts_);
}

void MHHMM::DeepCopy(const ConditionalMultinomialParam<int>& original, \
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

void MHHMM::PersistParams(const string& outputFilename) {
    string translationFilename = outputFilename + string(".t");
    MultinomialParams::PersistParams(translationFilename, tParams_, vocabEncoder_, true, true);
    string transitionFilename = outputFilename + string(".a");
    MultinomialParams::PersistParams(transitionFilename, aParams_, vocabEncoder_, false, false);
}

vector<int> MHHMM::ReadSelectedPath(VectorFst<LogArc>& sampled) {
    vector<int> src_sent;
    StateIterator<VectorFst<LogArc> > siter(sampled);
    while (!siter.Done()) {
        ArcIterator<LogVectorFst> aiter(sampled, siter.Value());
        if (!aiter.Done()) {
            src_sent.push_back(aiter.Value().ilabel);
            aiter.Next();
            if (!aiter.Done()) {
                cerr << "there are more than one arc leaving a state!\n";
            }
        }
        siter.Next();
    }
    return src_sent;
}

void MHHMM::CreateTgtFst(const vector<int>& sent, VectorFst<FstUtils::LogQuadArc>& tgt_fst) {
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

void MHHMM::CreateTranslationGrammar(const vector<int>& tgt_sent, \
        const vector<int>& src_sent, \
        VectorFst<FstUtils::LogQuadArc>& grammar) {
    set<int> tgt_tokens(tgt_sent.begin(), tgt_sent.end());
    assert(src_sent[0] == NULL_SRC_TOKEN_ID);
    int state_id = grammar.AddState();
    assert(state_id == 0);
    for (vector<int>::const_iterator siter = src_sent.begin(); \
            siter != src_sent.end(); ++siter) {
        for (set<int>::const_iterator titer = tgt_tokens.begin(); \
                titer != tgt_tokens.end(); ++titer) {
            grammar.AddArc(state_id, \
                FstUtils::LogQuadArc(*titer, *siter, \
                FstUtils::EncodeQuad(0, 0, 0, tParams_[*siter][*titer]), state_id)); 
        }
    }
    grammar.SetStart(state_id);
    grammar.SetFinal(state_id, FstUtils::LogQuadWeight::One());
    ArcSort(&grammar, ILabelCompare<FstUtils::LogQuadArc>());
}

float MHHMM::ComputeAlignmentScore(\
            const VectorFst<FstUtils::LogQuadArc>& tgt_fst, \
            const vector<int>& tgt_sent, \
            const vector<int>& src_sent, \
            VectorFst<FstUtils::LogQuadArc>& alignment, \
            vector<FstUtils::LogQuadWeight>& alpha, \
            vector<FstUtils::LogQuadWeight>& beta) {
    VectorFst<FstUtils::LogQuadArc> grammar;
    VectorFst<FstUtils::LogQuadArc> transition;
    CreateTranslationGrammar(tgt_sent, src_sent, grammar);
    CreateTransitionFst(src_sent, transition);
    VectorFst<FstUtils::LogQuadArc> temp;
    Compose(tgt_fst, grammar, &temp);
    Compose(temp, transition, &alignment);
    ShortestDistance(alignment, &alpha, false);
    ShortestDistance(alignment, &beta, true);
    float prob, dum;
    FstUtils::DecodeQuad(beta[alignment.Start()], dum, dum, dum, prob);
    return prob;
}

void MHHMM::CreateTransitionFst(const vector<int>& src_sent, \
                                VectorFst<FstUtils::LogQuadArc>& transition) {
    assert(src_sent.size() > 0 && src_sent[0] == NULL_SRC_TOKEN_ID);
    for (size_t i = 0; i < src_sent.size(); i++) {
        int state_id = transition.AddState();
        assert(i == state_id);
    }
    vector<vector<double> > normalized_prob;
    for (size_t i = 0; i < src_sent.size(); ++i) {
        vector<double> zero(src_sent.size(), 0);
        normalized_prob.push_back(zero);
    }
    NormalizePartialParams(src_sent.size(), normalized_prob);
    for (int i = 0; i < src_sent.size(); i++) {
        if (i == 0) {
            transition.SetStart(i);
        } 
        else {
            transition.SetFinal(i, FstUtils::LogQuadWeight::One());
        }
        int prev_alignment = i;
        transition.AddArc(i, \
                FstUtils::LogQuadArc(src_sent[0], src_sent[0], \
                    FstUtils::EncodeQuad(0, i, prev_alignment, normalized_prob[prev_alignment][i]), i));
        for(int j = 1; j < src_sent.size(); ++j) {
            transition.AddArc(i, FstUtils::LogQuadArc(src_sent[j], src_sent[j], \
                        FstUtils::EncodeQuad(0, j, prev_alignment, normalized_prob[prev_alignment][j]), j));
        }
    }
    ArcSort(&transition, ILabelCompare<FstUtils::LogQuadArc>());
}

void MHHMM::GatherPartialCounts(\
        const VectorFst<FstUtils::LogQuadArc>& alignment, \
        const vector<FstUtils::LogQuadWeight>& alpha, \
        const vector<FstUtils::LogQuadWeight>& beta) {
    float total_prob, dum;
    FstUtils::DecodeQuad(beta[alignment.Start()], dum, dum, dum, total_prob);
    StateIterator<VectorFst<FstUtils::LogQuadArc> > siter(alignment);
    while (!siter.Done()) {
        ArcIterator<VectorFst<FstUtils::LogQuadArc> > \
            aiter(alignment, siter.Value());
        while (!aiter.Done()) {
            int t = aiter.Value().ilabel;
            int s = aiter.Value().olabel;
            int from = siter.Value();
            int to = aiter.Value().nextstate;
            float ftgt_pos, fsrc_pos, fprev_pos, arc_weight;
            FstUtils::DecodeQuad(aiter.Value().weight, ftgt_pos, fsrc_pos, fprev_pos, arc_weight);
            int src_pos = (int) fsrc_pos;
            int tgt_pos = (int) ftgt_pos;
            int prev_pos = (int) fprev_pos;
            float a, b;
            FstUtils::DecodeQuad(alpha[from], dum, dum, dum, a);
            FstUtils::DecodeQuad(beta[to], dum, dum, dum, b);
            float norm_prob = a + arc_weight + b - total_prob;
            tFractionalCounts_[s][t] = \
                Plus(FstUtils::LogWeight(tFractionalCounts_[s][t]), \
                        FstUtils::LogWeight(norm_prob)).Value();
            aFractionalCounts_[prev_pos][src_pos] = \
                Plus(FstUtils::LogWeight(\
                            aFractionalCounts_[prev_pos][src_pos]), \
                        FstUtils::LogWeight(norm_prob)).Value();
            aiter.Next();
        }
        siter.Next();
    }
}

double MHHMM::FindLogMax(vector<double>& log_probs) {
    vector<double>::iterator iter = log_probs.begin();
    double max = *iter; 
    for (; iter != log_probs.end(); ++iter) { 
       if (*iter > max) {
          max = *iter;
       }
    }
    return max;
}

double MHHMM::LogSum(vector<double>& array) {
    double max = FindLogMax(array);
    double sum = 0;
    for (int i = 0; i < (int) array.size(); ++i) {
       sum += exp(array[i] - max); 
    }
    return max + log(sum);
}

void MHHMM::NormalizePartialParams(unsigned int n, vector<vector<double> >& prob) {
    for (int i = 0; i < n; ++i) {
        prob[0][i] = -aParams_[0][i]; 
    }
    vector<double> elems(prob[0].begin(), prob[0].begin() + n);
    double log_sum = LogSum(elems);
    for (int i = 0; i < n; ++i) {
        prob[0][i] -= log_sum; 
        prob[0][i] *= -1;
    }
    for (int i = 1; i < n; ++i) {
        for (int j = 1; j < n; ++j) {
            prob[i][j] = -aParams_[i][j]; 
        }
        prob[i][0] = prob[i][i];
        elems = prob[i];
        log_sum = LogSum(elems);
        for (int j = 0; j < n; ++j) {
            prob[i][j] -= log_sum; 
            prob[i][j] *= -1;
        }
    }
}
