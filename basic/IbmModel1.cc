#include "IbmModel1.h"

#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;
using namespace fst;

IbmModel1::IbmModel1(const string& _bitextFilename,
                     const string& _outputFilenamePrefix,
                     const LearningInfo& _learningInfo, 
                     const bool use_lattice) {
    bitextFilename = _bitextFilename;
    outputPrefix = _outputFilenamePrefix;
    learningInfo = _learningInfo;
    vocabEncoder.useUnk = false;

    if (use_lattice) {
        ConstructWithLattice();
    }
    else {
        Construct();
    }
    // initialize the model parameters
    cerr << "init model1 params" << endl;
    InitParams();

    stringstream initialModelFilename;
    initialModelFilename << outputPrefix << ".param.init";
    PersistParams(initialModelFilename.str());
}

// initialize model 1 scores
void IbmModel1::Construct() {
    // read encoded training data
    vocabEncoder.ReadParallelCorpus(bitextFilename, srcSents, tgtSents, NULL_SRC_TOKEN_STRING);
    assert(srcSents.size() > 0 && srcSents.size() == tgtSents.size());  
}

void IbmModel1::ConstructWithLattice() {
   vocabEncoder.ReadParallelCorpus(bitextFilename, \
                                   srcSents, tgtLatticePaths, NULL_SRC_TOKEN_STRING);

   assert(srcSents.size() > 0 && srcSents.size() == tgtLatticePaths.size());
   FstUtils::LoadLatticeFsts(tgtLatticePaths, tgtSents, tgtFsts, vocabEncoder);
}

void IbmModel1::Train() {
  CreateTgtFsts();

  // training iterations
  cerr << "train!" << endl;
  LearnParameters();

  // persist parameters
  cerr << "persist" << endl;
  PersistParams(outputPrefix + ".param.final");
}

void IbmModel1::TrainWithLattices() {
  cerr << "train!" << endl;
  LearnParameters();

  // persist parameters
  cerr << "persist" << endl;
  PersistParams(outputPrefix + ".param.final");
}

void IbmModel1::CreateTgtFsts() {
    for(unsigned i = 0; i < tgtSents.size(); i++) {
        // read the list of integers representing target tokens
        vector<int> &intTokens = tgtSents[i];
        // create the fst
        VectorFst<FstUtils::LogArc> tgtFst;
        int statesCount = intTokens.size() + 1;
        for(int stateId = 0; stateId < intTokens.size() + 1; stateId++) {
            int temp = tgtFst.AddState();
            assert(temp == stateId);
            if(stateId == 0) {
                continue;
            }
            else {
                tgtFst.AddArc(stateId - 1, FstUtils::LogArc(intTokens[stateId - 1], \
                              intTokens[stateId - 1], 0, stateId));
            }
        }
        tgtFst.SetStart(0);
        tgtFst.SetFinal(intTokens.size(), 0);
        ArcSort(&tgtFst, ILabelCompare<FstUtils::LogArc>());
        tgtFsts.push_back(tgtFst);
    }
}

// normalizes the parameters such that \sum_t p(t|s) = 1 \forall s
void IbmModel1::NormalizeParams() {
    MultinomialParams::NormalizeParams<int>(params);
}

void IbmModel1::PersistParams(const string& outputFilename) {
    MultinomialParams::PersistParams(outputFilename, params, vocabEncoder, true, true);
}

void IbmModel1::InitParams() {
    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        vector<int> &tgtTokens = tgtSents[sentId], \
                    &srcTokens = srcSents[sentId];
        // for each srcToken
        for(int i = 0; i < srcTokens.size(); i++) {
            int srcToken = srcTokens[i];
            // get the corresponding map of tgtTokens (and the corresponding probabilities)
            map<int, double> &translations = params[srcToken];
            for (int j=0; j<tgtTokens.size(); j++) {
	            int tgtToken = tgtTokens[j];
	            if (translations.count(tgtToken) == 0) {
	                translations[tgtToken] = FstUtils::nLog(1/3.0);
	            } else {
	                translations[tgtToken] = Plus(\
                            FstUtils::LogWeight(translations[tgtToken]), \
                            FstUtils::LogWeight(FstUtils::nLog(1/3.0)) ).Value();
	            }
            }
        }
    }
    NormalizeParams();
}

void IbmModel1::CreatePerSentGrammarFsts(vector<VectorFst<FstUtils::LogArc> >& \
                                         perSentGrammarFsts) {
    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        vector<int> &srcTokens = srcSents[sentId];
        vector<int> &tgtTokensVector = tgtSents[sentId];
        set<int> tgtTokens(tgtTokensVector.begin(), tgtTokensVector.end());
        // create the fst
        VectorFst< FstUtils::LogArc > grammarFst;
        int stateId = grammarFst.AddState();
        assert(stateId == 0);
        for(vector<int>::const_iterator srcTokenIter = srcTokens.begin(); \
                                        srcTokenIter != srcTokens.end(); \
                                        srcTokenIter++) {
            for(set<int>::const_iterator tgtTokenIter = tgtTokens.begin(); \
                                         tgtTokenIter != tgtTokens.end(); \
                                         tgtTokenIter++) {
	            grammarFst.AddArc(stateId, \
                                  FstUtils::LogArc(*tgtTokenIter, \
                                                   *srcTokenIter, \
                                                   params[*srcTokenIter][*tgtTokenIter], \
                                                   stateId));	
            }
        }
        grammarFst.SetStart(stateId);
        grammarFst.SetFinal(stateId, 0);
        ArcSort(&grammarFst, ILabelCompare<FstUtils::LogArc>());
        perSentGrammarFsts.push_back(grammarFst);
    }
}

// zero all parameters
void IbmModel1::ClearParams() {
    for (map<int, MultinomialParams::MultinomialParam>::iterator \
            srcIter = params.params.begin(); srcIter != params.params.end(); srcIter++) {
        for (MultinomialParams::MultinomialParam::iterator \
            tgtIter = srcIter->second.begin(); \
            tgtIter != srcIter->second.end(); tgtIter++) {
            tgtIter->second = FstUtils::LOG_ZERO;
        }
    }
}

void IbmModel1::LearnParameters() {
    clock_t compositionClocks = 0, \
            forwardBackwardClocks = 0, \
            updatingFractionalCountsClocks = 0, \
            grammarConstructionClocks = 0, \
            normalizationClocks = 0;
    clock_t t00 = clock();
    do {
        clock_t t05 = clock();
        vector<VectorFst<FstUtils::LogArc > > perSentGrammarFsts;
        CreatePerSentGrammarFsts(perSentGrammarFsts);
        grammarConstructionClocks += clock() - t05;

        clock_t t10 = clock();
        float logLikelihood = 0;
    
        ClearParams();
        // iterate over sentences
        int sentsCounter = 0;
        for(vector<VectorFst<FstUtils::LogArc> >::const_iterator \
                tgtIter = tgtFsts.begin(), \
                grammarIter = perSentGrammarFsts.begin(); 
	            tgtIter != tgtFsts.end() && grammarIter != perSentGrammarFsts.end(); \
	            tgtIter++, grammarIter++) {
            // build the alignment fst
            clock_t t20 = clock();
            VectorFst<FstUtils::LogArc> tgtFst = *tgtIter, \
                                        perSentGrammarFst = *grammarIter, \
                                        alignmentFst;
            Compose(tgtFst, perSentGrammarFst, &alignmentFst);
            compositionClocks += clock() - t20;
      
            clock_t t30 = clock();
            vector<FstUtils::LogWeight> alphas, betas;
            ShortestDistance(alignmentFst, &alphas, false);
            ShortestDistance(alignmentFst, &betas, true);
            float fSentLogLikelihood = betas[alignmentFst.Start()].Value();
      
            forwardBackwardClocks += clock() - t30;
            clock_t t40 = clock();
            for (int stateId = 0; stateId < alignmentFst.NumStates(); stateId++) {
	            for (ArcIterator<VectorFst<FstUtils::LogArc> > arcIter(alignmentFst,\
                         stateId); !arcIter.Done(); arcIter.Next()) {
                    int srcToken = arcIter.Value().olabel, \
                        tgtToken = arcIter.Value().ilabel;
	                int fromState = stateId, \
                        toState = arcIter.Value().nextstate;
	  
	                FstUtils::LogWeight currentParamLogProb = arcIter.Value().weight;
	                FstUtils::LogWeight unnormalizedPosteriorLogProb = \
                        Times(Times(alphas[fromState], currentParamLogProb), \
                              betas[toState]);
	                float fNormalizedPosteriorLogProb = \
                        unnormalizedPosteriorLogProb.Value() - fSentLogLikelihood;
	                params[srcToken][tgtToken] = Plus(\
                        FstUtils::LogWeight(params[srcToken][tgtToken]), \
                        FstUtils::LogWeight(fNormalizedPosteriorLogProb)).Value();
	  
	            }
            }
            updatingFractionalCountsClocks += clock() - t40;
	        logLikelihood += fSentLogLikelihood;
            // logging
            if (++sentsCounter % 1000 == 0) {
	            cerr << sentsCounter 
                     << " sents processed. iterationLoglikelihood = " 
                     << logLikelihood <<  endl;
            }
        }
        // normalize fractional counts such that \sum_t p(t|s) = 1 \forall s
        clock_t t50 = clock();
        NormalizeParams();
        normalizationClocks += clock() - t50;
        // create the new grammar
        clock_t t60 = clock();
        grammarConstructionClocks += clock() - t60;

        if(learningInfo.iterationsCount % learningInfo.persistParamsAfterNIteration == 0 && 
	        learningInfo.iterationsCount != 0) { 
            cerr << "persisting params:" << endl;
            stringstream filename;
            filename << outputPrefix << ".param." << learningInfo.iterationsCount;
            PersistParams(filename.str());
        }

        // logging
        cerr << "iteration # " 
            << learningInfo.iterationsCount 
            << " - total loglikelihood = " << logLikelihood << endl;
        
        // update learningInfo
        learningInfo.logLikelihood.push_back(logLikelihood);
        learningInfo.iterationsCount++;
    
    // check for convergence
  } while(!learningInfo.IsModelConverged());

  // logging
  cerr << endl;
  cerr << "trainTime        = " 
       << (float) (clock() - t00) / CLOCKS_PER_SEC << " sec." << endl;
  cerr << "compositionTime  = " 
       << (float) compositionClocks / CLOCKS_PER_SEC << " sec." << endl;
  cerr << "forward/backward = " 
       << (float) forwardBackwardClocks / CLOCKS_PER_SEC << " sec." << endl;
  cerr << "fractionalCounts = " 
       << (float) updatingFractionalCountsClocks / CLOCKS_PER_SEC << " sec." << endl;
  cerr << "normalizeClocks  = " 
       << (float) normalizationClocks / CLOCKS_PER_SEC << " sec." << endl;
  cerr << "grammarConstruct = " 
       << (float) grammarConstructionClocks / CLOCKS_PER_SEC << " sec." << endl;
  cerr << endl;
}

/*
// given the current model, align the corpus
void IbmModel1::Align() {
    Align(outputPrefix + ".train.align");
}

void IbmModel1::Align(const string &alignmentsFilename) {
    ofstream outputAlignments;
    outputAlignments.open(alignmentsFilename.c_str(), ios::out);

    vector<VectorFst< FstUtils::LogArc > > perSentGrammarFsts;
    CreatePerSentGrammarFsts(perSentGrammarFsts);
    vector< VectorFst <FstUtils::LogArc> > tgtFsts;
    CreateTgtFsts(tgtFsts);

    assert(tgtFsts.size() == srcSents.size());
    assert(perSentGrammarFsts.size() == srcSents.size());
    assert(tgtSents.size() == srcSents.size());

    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        vector<int> &srcSent = srcSents[sentId], \
                    &tgtSent = tgtSents[sentId];
        VectorFst<FstUtils::LogArc> &perSentGrammarFst = perSentGrammarFsts[sentId], \
                                    &tgtFst = tgtFsts[sentId], alignmentFst;
        // given a src token id, what are the possible src position (in this sentence)
        map<int, set<int> > srcTokenToSrcPos;
        for(unsigned srcPos = 0; srcPos < srcSent.size(); srcPos++) {
            srcTokenToSrcPos[srcSent[srcPos]].insert(srcPos);
        }
    
        // build alignment fst and compute potentials
        Compose(tgtFst, perSentGrammarFst, &alignmentFst);
        vector<FstUtils::LogWeight> alphas, betas;
        ShortestDistance(alignmentFst, &alphas, false);
        ShortestDistance(alignmentFst, &betas, true);
        double fSentLogLikelihood = betas[alignmentFst.Start()].Value();
    
        VectorFst<FstUtils::StdArc> alignmentFstProbsWithPathProperty, \
                                    bestAlignment, \
                                    corrected;
        ArcMap(alignmentFst, &alignmentFstProbsWithPathProperty, \
               FstUtils::LogToTropicalMapper());
        ShortestPath(alignmentFstProbsWithPathProperty, &bestAlignment);
   
        stringstream alignmentsLine;
        int startState = bestAlignment.Start();
        int currentState = startState;
        int tgtPos = 0;
        while(bestAlignment.Final(currentState) == FstUtils::LogWeight::Zero()) {
            // get hold of the arc
            ArcIterator<VectorFst<FstUtils::StdArc > > aiter(bestAlignment, currentState);
            // identify the next state
            int nextState = aiter.Value().nextstate;
            // skip epsilon arcs
            if(aiter.Value().ilabel == FstUtils::EPSILON && \
               aiter.Value().olabel == FstUtils::EPSILON) {
	            currentState = nextState;
	            continue;
            }
            // update tgt pos
            tgtPos++;
            // find src pos
            int srcPos = 0;
            assert(srcTokenToSrcPos[aiter.Value().olabel].size() > 0);
            if(srcTokenToSrcPos[aiter.Value().olabel].size() == 1) {
	            srcPos = *(srcTokenToSrcPos[aiter.Value().olabel].begin());
            } else {
	            float distortion = 100;
	            for(set<int>::iterator srcPosIter = \
                    srcTokenToSrcPos[aiter.Value().olabel].begin(); \
                    srcPosIter != srcTokenToSrcPos[aiter.Value().olabel].end(); \
                    srcPosIter++) {
	                if(*srcPosIter - tgtPos < distortion) {
	                    srcPos = *srcPosIter;
	                    distortion = abs(*srcPosIter - tgtPos);
	                }
	            }
            }
            if(srcPos != 0) {
	            alignmentsLine << (srcPos - 1) << "-" << (tgtPos - 1) << " ";
            }
            // this state shouldn't have other arcs!
            aiter.Next();
            assert(aiter.Done());
            // move forward to the next state
            currentState = nextState;
        }
        alignmentsLine << endl;
        // write the best alignment to file
        outputAlignments << alignmentsLine.str();
    }
    outputAlignments.close();
}
*/
