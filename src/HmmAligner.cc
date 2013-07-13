#include "HmmAligner.h"

using namespace std;
using namespace fst;
using namespace MultinomialParams;
// using namespace boost;

HmmAligner::HmmAligner(const string& _bitextFilename,
                       const string& _outputFilenamePrefix,
                       const LearningInfo& _learningInfo,
                       const string& _tParamsFile,
                       const bool use_lattice) {
    // set member variables
    outputPrefix = _outputFilenamePrefix;
    learningInfo = _learningInfo;

    // encode training data
    vocabEncoder.useUnk = false; 

    if (use_lattice) {
        vocabEncoder.ReadParallelCorpus(_bitextFilename, \
                                        srcSents, \
                                        tgtLatticePaths, \
                                        NULL_SRC_TOKEN_STRING);
        assert(srcSents.size() > 0 && srcSents.size() == tgtLatticePaths.size());
        FstUtils::LoadLatticeFsts(tgtLatticePaths, tgtSents, tgtFsts, vocabEncoder);
    }
    else {
        vocabEncoder.ReadParallelCorpus(_bitextFilename, \
                                        srcSents, \
                                        tgtSents, \
                                        NULL_SRC_TOKEN_STRING);
        assert(srcSents.size() > 0 && srcSents.size() == tgtSents.size());
    }

   cerr << "Initialize T params using " << _tParamsFile << endl;
   LoadInitTParams(_tParamsFile);
   InitJustAParams();

   stringstream initialModelFilename;
   initialModelFilename << outputPrefix << ".param.init";
   PersistParams(initialModelFilename.str());

   CreatePerSentGrammarFsts();
}

HmmAligner::HmmAligner(const string& tParamsFile, 
                       const string& aParamsFile,  
                       const string& testFile) {
   LoadInitTParams(tParamsFile);
   LoadInitAParams(aParamsFile);

   // encode training data
   vocabEncoder.useUnk = false; 
   vocabEncoder.ReadParallelCorpus(testFile,
                                   srcSents, 
                                   tgtLatticePaths, 
                                   NULL_SRC_TOKEN_STRING);
   assert(srcSents.size() > 0 && srcSents.size() == tgtLatticePaths.size());
   FstUtils::LoadLatticeFsts(tgtLatticePaths, tgtSents, tgtFsts, vocabEncoder);
   CreatePerSentGrammarFsts();
}

void HmmAligner::LoadInitAParams(const string& initAparams) {
    ifstream infile(initAparams.c_str());
    int src, tgt;
    double t;
    while (infile >> tgt >> src >> t) {
        map<int, double> &aParamsGivenS_i = aFractionalCounts[src];
        aFractionalCounts[src][tgt] = t;
    }
    infile.close();
}

void HmmAligner::LoadInitTParams(const string& initTparams) {
    ifstream infile(initTparams.c_str());
    string srcToken, tgtToken;
    double t;
    while (infile >> tgtToken >> srcToken >> t) {
        int src = vocabEncoder.Encode(srcToken, false);
        int tgt = vocabEncoder.Encode(tgtToken, false); 
        map<int, double> &tParamsGivenS_i = tFractionalCounts[src];
        tFractionalCounts[src][tgt] = t;
    } 
    infile.close();
}

void HmmAligner::Train() {
    CreateTgtFsts();
    cerr << "train!" << endl;
    LearnParameters();
}

void HmmAligner::TrainWithLattices() {
    cerr << "train!" << endl;
    LearnParameters();
}

void HmmAligner::CreateTgtFsts() {
    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        vector<int> &intTokens = tgtSents[sentId];
        VectorFst< FstUtils::LogQuadArc > tgtFst;
        CreateTgtFst(intTokens, tgtFst);
        tgtFsts.push_back(tgtFst);
    }
}

void HmmAligner::CreateTgtFst(const vector<int> tgtTokens, \
                              VectorFst<FstUtils::LogQuadArc>& tgtFst) {
    assert(tgtFst.NumStates() == 0);
    int statesCount = tgtTokens.size() + 1;
    for(int stateId = 0; stateId < tgtTokens.size() + 1; stateId++) {
        int temp = tgtFst.AddState();
        assert(temp == stateId);
        if(stateId == 0) continue;
        int tgtPos = stateId;
        tgtFst.AddArc(stateId-1, 
		              FstUtils::LogQuadArc(tgtTokens[stateId - 1], 
			                               tgtTokens[stateId-1], 
			                               FstUtils::EncodeQuad(tgtPos, 0, 0, 0), 
                        			       stateId));
    }
    tgtFst.SetStart(0);
    tgtFst.SetFinal(tgtTokens.size(), FstUtils::LogQuadWeight::One());
    ArcSort(&tgtFst, ILabelCompare<FstUtils::LogQuadArc>());
}


// src fsts are 1st order markov models
void HmmAligner::CreateSrcFsts(vector<VectorFst<FstUtils::LogQuadArc > >& srcFsts) {
    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        vector<int> &intTokens = srcSents[sentId];
        if(intTokens[0] != NULL_SRC_TOKEN_ID) {
            intTokens.insert(intTokens.begin(), NULL_SRC_TOKEN_ID);
        }
        // create the fst
        VectorFst<FstUtils::LogQuadArc> srcFst;
        Create1stOrderSrcFst(intTokens, srcFst);
        srcFsts.push_back(srcFst);
    }
}

void HmmAligner::CreatePerSentGrammarFsts() {
    perSentGrammarFsts.clear();
    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        vector<int> &srcTokens = srcSents[sentId];
        vector<int> &tgtTokens = tgtSents[sentId];
        assert(srcTokens[0] == NULL_SRC_TOKEN_ID);
        VectorFst<FstUtils::LogQuadArc> perSentGrammarFst;
        CreatePerSentGrammarFst(srcTokens, tgtTokens, perSentGrammarFst);
        perSentGrammarFsts.push_back(perSentGrammarFst);
    }
}

void HmmAligner::CreatePerSentGrammarFst(vector<int> &srcTokens, 
                                         vector<int> &tgtTokensVector, 
                                         VectorFst<FstUtils::LogQuadArc>& perSentGrammarFst) {
    set<int> tgtTokens(tgtTokensVector.begin(), tgtTokensVector.end());
    // allow null alignments
    assert(srcTokens[0] == NULL_SRC_TOKEN_ID);
    
    // create the fst
    int stateId = perSentGrammarFst.AddState();
    assert(stateId == 0);
    for(vector<int>::const_iterator srcTokenIter = srcTokens.begin(); 
                                    srcTokenIter != srcTokens.end(); 
                                    srcTokenIter++) {
        for(set<int>::const_iterator tgtTokenIter = tgtTokens.begin(); 
                                     tgtTokenIter != tgtTokens.end(); 
                                     tgtTokenIter++) {
            perSentGrammarFst.AddArc(stateId, 
                                     FstUtils::LogQuadArc(*tgtTokenIter, *srcTokenIter, 
                                                          FstUtils::EncodeQuad(0, 0, 0, tFractionalCounts[*srcTokenIter][*tgtTokenIter]), stateId));	
        }
    }
    perSentGrammarFst.SetStart(stateId);
    perSentGrammarFst.SetFinal(stateId, FstUtils::LogQuadWeight::One());
    ArcSort(&perSentGrammarFst, ILabelCompare<FstUtils::LogQuadArc>());
}								       

void HmmAligner::NormalizeFractionalCounts() {
    MultinomialParams::NormalizeParams(aFractionalCounts);
    MultinomialParams::NormalizeParams(tFractionalCounts);
}

void HmmAligner::PersistParams(const string& outputFilename) {
    string translationFilename = outputFilename + string(".t");
    MultinomialParams::PersistParams(translationFilename, tFractionalCounts, vocabEncoder, true, true);
    string transitionFilename = outputFilename + string(".a");
    MultinomialParams::PersistParams(transitionFilename, aFractionalCounts, vocabEncoder, false, false);
}

void HmmAligner::InitJustAParams() {
    for(int sentId = 0; sentId < srcSents.size(); sentId++) {  
        vector<int> &srcTokens = srcSents[sentId];
        // we want to allow target words to align to NULL (which has srcTokenId = 1).
        if(srcTokens[0] != NULL_SRC_TOKEN_ID) {
            srcTokens.insert(srcTokens.begin(), NULL_SRC_TOKEN_ID);
        }
        // for each srcToken
        for(int i = 1; i < srcTokens.size(); ++i) {
            for(int k = 1; k < srcTokens.size(); ++k) {
                if (aFractionalCounts[k].count(i) == 0) {
                    aFractionalCounts[k][i] = FstUtils::nLog(1/3.0);
                }
                else {
                    aFractionalCounts[k][i] = Plus(FstUtils::LogWeight(aFractionalCounts[k][i]), FstUtils::LogWeight(FstUtils::nLog(1/3.0))).Value();
                }
            }
        }
        for (int k = 0; k < srcTokens.size(); ++k) {
            if (!k) {
                for (int i = 0; i < srcTokens.size(); ++i) {
                    if (aFractionalCounts[k].count(i) == 0) {
                        aFractionalCounts[k][i] = FstUtils::nLog(1/3.0);
                    }
                    else {
                        aFractionalCounts[k][i] = Plus(FstUtils::LogWeight(aFractionalCounts[k][i]), FstUtils::LogWeight(FstUtils::nLog(1/3.0))).Value();   
                    }
                } 
            }
            else {
                if (aFractionalCounts[k].count(k) == 0) {
                    aFractionalCounts[k][k] = FstUtils::nLog(1/3.0);
                }
                else {
                    aFractionalCounts[k][k] = Plus(FstUtils::LogWeight(aFractionalCounts[k][k]), FstUtils::LogWeight(FstUtils::nLog(1/3.0))).Value();   
                }
            }
        }
    }
    NormalizeFractionalCounts();
}

// assumptions:
// - first token in srcTokens is the NULL token (to represent null-alignments)
// - srcFst is assumed to be empty
//
// notes:
// - the structure of this FST is laid out such that each state encodes the previous non-null 
//   src position. the initial state is unique: it represents both the starting state the state
//   where all previous alignments are null-alignments.
// - if a source type is repeated, it will have multiple states corresponding to the different positions
// - the "1stOrder" part of the function name indicates this FST represents a first order markov process
//   for alignment transitions.
//
void HmmAligner::Create1stOrderSrcFst(const vector<int>& srcTokens, VectorFst<FstUtils::LogQuadArc>& srcFst) {
    // enforce assumptions
    assert(srcTokens.size() > 0 && srcTokens[0] == NULL_SRC_TOKEN_ID);
    assert(srcFst.NumStates() == 0);

    // create one state per src position
    for(int i = 0; i < srcTokens.size(); i++) {
        int stateId = srcFst.AddState();
        // assumption that AddState() first returns a zero then increment ones
        assert(i == stateId);
    }
    // for each state
    for(int i = 0; i < srcTokens.size(); i++) {
        // set the initial/final states
        if(i == 0) {
            srcFst.SetStart(i);
        } else {
            srcFst.SetFinal(i, FstUtils::LogQuadWeight::One());
        }
        /* we don't allow prevAlignment to be null alignment in our markov model. 
           If a null alignment happens after alignment = 5, we use 5 as prevAlignment, not the null alignment. 
           If null alignment happens before any non-null alignment, we use a special src position INITIAL_SRC_POS to indicate the prevAlignment */
        // each state can go to itself with the null src token
        srcFst.AddArc(i, FstUtils::LogQuadArc(srcTokens[0], srcTokens[0], FstUtils::EncodeQuad(0, i, i, aFractionalCounts[i][i]), i));
        // each state can go to states representing non-null alignments
        for(int j = 1; j < srcTokens.size(); j++) {
            srcFst.AddArc(i, FstUtils::LogQuadArc(srcTokens[j], srcTokens[j], FstUtils::EncodeQuad(0, j, i, aFractionalCounts[i][j]), j));
        }
    }
    // arc sort to enable composition
    ArcSort(&srcFst, ILabelCompare<FstUtils::LogQuadArc>());
}

void HmmAligner::ClearFractionalCounts() {
    MultinomialParams::ClearParams(tFractionalCounts, learningInfo.smoothMultinomialParams);
    MultinomialParams::ClearParams(aFractionalCounts, learningInfo.smoothMultinomialParams);
}

void HmmAligner::BuildAlignmentFst(
                 const VectorFst< FstUtils::LogQuadArc > &tgtFst,
				 const VectorFst< FstUtils::LogQuadArc > &perSentGrammarFst,
				 const VectorFst< FstUtils::LogQuadArc > &srcFst, 
				 VectorFst< FstUtils::LogQuadArc > &alignmentFst) {
    VectorFst< FstUtils::LogQuadArc > temp;
    Compose(tgtFst, perSentGrammarFst, &temp);
    Compose(temp, srcFst, &alignmentFst);  
}

void HmmAligner::LearnParameters() {
    clock_t compositionClocks = 0, 
            forwardBackwardClocks = 0, 
            updatingFractionalCountsClocks = 0, 
            normalizationClocks = 0;

    clock_t t00 = clock();
    do {
        clock_t t05 = clock();
        cerr << "create src fsts" << endl;
        vector< VectorFst <FstUtils::LogQuadArc> > srcFsts;
        CreateSrcFsts(srcFsts);
    
        cerr << "created src fsts" << endl;
        clock_t t10 = clock();
        float logLikelihood = 0; 
    
        ClearFractionalCounts();
        cerr << "cleared fractional counts vector" << endl;

        // iterate over sentences
        int sentsCounter = 0;
        for(vector< VectorFst< FstUtils::LogQuadArc > >::const_iterator tgtIter = tgtFsts.begin(), 
                                                                        srcIter = srcFsts.begin(), 
                                                                        perSentGrammarIter = perSentGrammarFsts.begin(); 
            tgtIter != tgtFsts.end() && 
            srcIter != srcFsts.end() && 
            perSentGrammarIter != perSentGrammarFsts.end(); 
	        tgtIter++, srcIter++, perSentGrammarIter++, sentsCounter++) {

            clock_t t20 = clock();
            const VectorFst< FstUtils::LogQuadArc > &tgtFst = *tgtIter, 
                                                    &srcFst = *srcIter, 
                                                    &perSentGrammarFst = *perSentGrammarIter;
            VectorFst< FstUtils::LogQuadArc > alignmentFst;
            BuildAlignmentFst(tgtFst, perSentGrammarFst,  srcFst, alignmentFst);
            compositionClocks += clock() - t20;
     
            // run forward/backward for this sentence
            clock_t t30 = clock();
            vector<FstUtils::LogQuadWeight> alphas, betas;
            ShortestDistance(alignmentFst, &alphas, false);
            ShortestDistance(alignmentFst, &betas, true);

            float fSentLogLikelihood, dummy;
            FstUtils::DecodeQuad(betas[alignmentFst.Start()], 
			                     dummy, 
                                 dummy, 
                                 dummy, 
                                 fSentLogLikelihood);
            if(std::isnan(fSentLogLikelihood) || std::isinf(fSentLogLikelihood)) {
	            cerr << ": sent #" << sentsCounter << " give a sent loglikelihood of " << fSentLogLikelihood << endl;
	            assert(false);
            }
            forwardBackwardClocks += clock() - t30;
            if(learningInfo.debugLevel >= DebugLevel::SENTENCE) {
	            cerr << "fSentLogLikelihood = " << fSentLogLikelihood << endl;
            }
            // compute and accumulate fractional counts for model parameters
            clock_t t40 = clock();
            for (int stateId = 0; stateId < alignmentFst.NumStates() ;stateId++) {
	            for (ArcIterator<VectorFst< FstUtils::LogQuadArc > > arcIter(alignmentFst, stateId); !arcIter.Done(); arcIter.Next()) {
                    // decode arc information
                    int srcToken = arcIter.Value().olabel, 
                        tgtToken = arcIter.Value().ilabel;
	                int fromState = stateId, 
                        toState = arcIter.Value().nextstate;
	                float fCurrentTgtPos,
                          fCurrentSrcPos, 
                          fPrevSrcPos, arcLogProb;
	                FstUtils::DecodeQuad(arcIter.Value().weight, fCurrentTgtPos, fCurrentSrcPos, fPrevSrcPos, arcLogProb);
	                int currentSrcPos = (int) fCurrentSrcPos, prevSrcPos = (int) fPrevSrcPos;

	                // probability of using this parameter given this sentence pair and the previous model
	                float alpha, beta, dummy;
	                FstUtils::DecodeQuad(alphas[fromState], dummy, dummy, dummy, alpha);
	                FstUtils::DecodeQuad(betas[toState], dummy, dummy, dummy, beta);
	                float fNormalizedPosteriorLogProb = (alpha + arcLogProb + beta) - fSentLogLikelihood;
	                if(std::isnan(fNormalizedPosteriorLogProb) || std::isinf(fNormalizedPosteriorLogProb)) {
	                    cerr << ": sent #" << sentsCounter << " give a normalized posterior logprob of  " << fNormalizedPosteriorLogProb << endl;
	                    assert(false);
	                }
	                // update tFractionalCounts
	                tFractionalCounts[srcToken][tgtToken] = Plus(FstUtils::LogWeight(tFractionalCounts[srcToken][tgtToken]), 
		                                                         FstUtils::LogWeight(fNormalizedPosteriorLogProb)).Value();
	                // update aFractionalCounts
	                aFractionalCounts[prevSrcPos][currentSrcPos] = Plus(FstUtils::LogWeight(aFractionalCounts[prevSrcPos][currentSrcPos]),
		                                                                FstUtils::LogWeight(fNormalizedPosteriorLogProb)).Value();
                }
            }
            updatingFractionalCountsClocks += clock() - t40;
      
            // update the iteration log likelihood with this sentence's likelihod
	        logLikelihood += fSentLogLikelihood;
            // logging
            if (sentsCounter > 0 && sentsCounter % 100 == 0 && learningInfo.debugLevel == DebugLevel::CORPUS) {
	            cerr << ".";
            }
        }

        if(learningInfo.debugLevel == DebugLevel::CORPUS) {
            cerr << "fractional counts collected from relevant sentences for this iteration." << endl;
        }
    
        clock_t t50 = clock();
        NormalizeFractionalCounts();
        normalizationClocks += clock() - t50;

        if( learningInfo.iterationsCount % learningInfo.persistParamsAfterNIteration == 0 && 
	        learningInfo.iterationsCount != 0) { 
            cerr << "persisting params:" << endl;
            stringstream filename;
            filename << outputPrefix << ".param." << learningInfo.iterationsCount;
            PersistParams(filename.str());
        }
    
        // create the new grammar
        clock_t t60 = clock();
        CreatePerSentGrammarFsts();

        cerr << "iterations # " << learningInfo.iterationsCount << " - total loglikelihood = " << logLikelihood << endl;
    
        // update learningInfo
        learningInfo.logLikelihood.push_back(logLikelihood);
        learningInfo.iterationsCount++;
    
        // check for convergence
    } while(!learningInfo.IsModelConverged());

    if (true) {
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
        cerr << endl;
    }
}

void HmmAligner::AlignWithLattice(const string& alignmentsFilename) {
    ofstream outputAlignments;
    outputAlignments.open(alignmentsFilename.c_str(), ios::out);
    for(unsigned sentId = 0; sentId < srcSents.size(); sentId++) {
        string alignmentsLine;
        vector<int> &srcSent = srcSents[sentId]; 
        vector<int> &tgtSent = tgtSents[sentId];
        fst::VectorFst<FstUtils::LogQuadArc>& tgtFst = tgtFsts[sentId];
        alignmentsLine = AlignSent(srcSent, tgtSent, tgtFst);
        outputAlignments << alignmentsLine;
    }
    outputAlignments.close();
}

string HmmAligner::AlignSent(vector<int> srcTokens, 
                             vector<int> tgtTokens, 
                             fst::VectorFst<FstUtils::LogQuadArc>& tgtFst) {
    // make sure the source sentnece is sane
    assert(srcTokens.size() > 0 && srcTokens[0] == NULL_SRC_TOKEN_ID);

    // build aGivenTS
    VectorFst<FstUtils::LogQuadArc> srcFst, alignmentFst, perSentGrammarFst;
    Create1stOrderSrcFst(srcTokens, srcFst);
    CreatePerSentGrammarFst(srcTokens, tgtTokens, perSentGrammarFst);
    BuildAlignmentFst(tgtFst, perSentGrammarFst, srcFst, alignmentFst);
    VectorFst< FstUtils::LogArc > alignmentFstProbs;
    ArcMap(alignmentFst, &alignmentFstProbs, FstUtils::LogQuadToLogPositionMapperLattice());
    // tropical has the path property
    VectorFst< FstUtils::StdArc > alignmentFstProbsWithPathProperty, bestAlignment;
    ArcMap(alignmentFstProbs, &alignmentFstProbsWithPathProperty, \
           FstUtils::LogToTropicalMapper());
    ShortestPath(alignmentFstProbsWithPathProperty, &bestAlignment);
    return FstUtils::PrintAlignmentWithLattice(bestAlignment, vocabEncoder);
}
