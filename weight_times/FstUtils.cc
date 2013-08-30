#include "FstUtils.h"
#include "fst/symbol-table.h"

using namespace fst;

const float FstUtils::LOG_PROBS_MUST_BE_GREATER_THAN_ME = -0.1;
const string FstUtils::LogArc::type = "FstUtils::LogArc";
const string FstUtils::StdArc::type = "FstUtils::StdArc";

FstUtils::LogPairWeight FstUtils::FstUtils::EncodePairInfinity() {
    return FstUtils::EncodePair(numeric_limits<float>::infinity(), numeric_limits<float>::infinity());
}

FstUtils::LogTripleWeight FstUtils::FstUtils::EncodeTripleInfinity() {
    return FstUtils::EncodeTriple(numeric_limits<float>::infinity(), numeric_limits<float>::infinity(), numeric_limits<float>::infinity());
}

FstUtils::LogQuadWeight FstUtils::FstUtils::EncodeQuadInfinity() {
    return FstUtils::EncodeQuad(numeric_limits<float>::infinity(), 
		                        numeric_limits<float>::infinity(), 
		                        numeric_limits<float>::infinity(), 
		                        numeric_limits<float>::infinity());
}

FstUtils::LogPairWeight FstUtils::FstUtils::EncodePair(float val1, float val2) {
    FstUtils::LogWeight v1, v2;
    v1 = val1;
    v2 = val2;
    return ProductWeight<FstUtils::LogWeight, FstUtils::LogWeight>(v1, v2);
}

FstUtils::LogTripleWeight FstUtils::FstUtils::EncodeTriple(float val1, float val2, float val3) {
    FstUtils::LogWeight v3;
    v3 = val3;
    return FstUtils::LogTripleWeight(FstUtils::EncodePair(val1, val2), v3);
}

FstUtils::LogQuadWeight FstUtils::FstUtils::EncodeQuad(float val1, float val2, float val3, float val4) {
    FstUtils::LogWeight v4;
    v4 = val4;
    return FstUtils::LogQuadWeight(FstUtils::EncodeTriple(val1, val2, val3), v4);
}

string FstUtils::PrintWeight(const FstUtils::TropicalWeight& w) {
    stringstream ss;
    ss << w.Value();
    return ss.str();
}

string FstUtils::PrintWeight(const FstUtils::LogWeight& w) {
    stringstream ss;
    ss << w.Value();
    return ss.str();
}

string FstUtils::PrintWeight(const ProductWeight<FstUtils::LogWeight, FstUtils::LogWeight>& w) {
    stringstream ss;
    ss << "(" << w.Value1().Value() << "," << w.Value2().Value() << ")";
    return ss.str();
}

string FstUtils::PrintWeight(const FstUtils::LogTripleWeight& w) {
    stringstream ss;
    ss << "(" << w.Value1().Value1().Value() 
       << "," << w.Value1().Value2().Value() 
       << "," << w.Value2().Value() 
       << ")";
    return ss.str();
}

string FstUtils::PrintWeight(const FstUtils::LogQuadWeight& w) {
    stringstream ss;
    ss << "(" << w.Value1().Value1().Value1().Value() 
       << "," << w.Value1().Value1().Value2().Value()
       << "," << w.Value1().Value2().Value()
       << "," << w.Value2().Value()
       << ")";
    return ss.str();
}

void FstUtils::LoadLatticeFst(const string tgtLatticePath, \
                              vector<int>& tgtSent, \
                              fst::VectorFst<FstUtils::LogArc>& tgtFst, \
                              VocabEncoder& vocabEncoder) {
   fst::StdVectorFst *input = fst::StdVectorFst::Read(tgtLatticePath);
   const SymbolTable *isyms = input -> InputSymbols();
   StateIterator<StdVectorFst> siter(*input);

   int currentState = siter.Value();
   tgtFst.SetStart(currentState);
   while (!siter.Done()) {
      currentState = siter.Value();
      tgtFst.AddState();
      ArcIterator<StdVectorFst> aiter(*input, currentState);
      while (!aiter.Done()) {
         int arcValue = aiter.Value().ilabel;
         string symbol = isyms -> Find(arcValue);
         int newValue = vocabEncoder.Encode(symbol, false);
         tgtSent.push_back(newValue);
         tgtFst.AddArc(siter.Value(), 
              FstUtils::LogArc(newValue, newValue, aiter.Value().weight.Value(), aiter.Value().nextstate)); 
         aiter.Next();
      }
      siter.Next();
   }
   tgtFst.SetFinal(currentState, 0);
   ArcSort(&tgtFst, ILabelCompare<FstUtils::LogArc>());
}

void FstUtils::LoadLatticeFsts(vector<string>& tgtLatticePaths, \
                               vector<vector<int> >& tgtSents, \
                               vector<fst::VectorFst<FstUtils::LogArc> >& tgtFsts,
                               VocabEncoder& vocabEncoder) {
    vector<string>::iterator piter = tgtLatticePaths.begin();
    for (; piter != tgtLatticePaths.end(); ++piter) {
        vector<int> tgtSent;
        fst::VectorFst<FstUtils::LogArc> tgtFst;
        LoadLatticeFst(*piter, tgtSent, tgtFst, vocabEncoder);
        tgtSents.push_back(tgtSent);
        tgtFsts.push_back(tgtFst);
    }
}

void FstUtils::LoadLatticeFst(const string tgtLatticePath, \
                              vector<int>& tgtSent, \
                              fst::VectorFst<FstUtils::LogQuadArc>& tgtFst, \
                              VocabEncoder& vocabEncoder) {
   fst::StdVectorFst *input = fst::StdVectorFst::Read(tgtLatticePath);
   const SymbolTable *isyms = input -> InputSymbols();
   StateIterator<StdVectorFst> siter(*input);

   int currentState = siter.Value();
   tgtFst.SetStart(currentState);
   while (!siter.Done()) {
      currentState = siter.Value();
      tgtFst.AddState();
      ArcIterator<StdVectorFst> aiter(*input, currentState);
      while (!aiter.Done()) {
         int arcValue = aiter.Value().ilabel;
         string symbol = isyms -> Find(arcValue);
         int newValue = vocabEncoder.Encode(symbol, false);
         tgtSent.push_back(newValue);
         tgtFst.AddArc(siter.Value(), 
                       FstUtils::LogQuadArc(newValue, \
                                  newValue, \
                                  FstUtils::EncodeQuad(0, 0, 0, aiter.Value().weight.Value()), 
                                  aiter.Value().nextstate));
         aiter.Next();
      }
      siter.Next();
   }
   tgtFst.SetFinal(currentState, FstUtils::LogQuadWeight::One());
   ArcSort(&tgtFst, ILabelCompare<FstUtils::LogQuadArc>());
}

void FstUtils::LoadLatticeFsts(vector<string>& tgtLatticePaths, \
                               vector<vector<int> >& tgtSents, \
                               vector<fst::VectorFst<FstUtils::LogQuadArc> >& tgtFsts,
                               VocabEncoder& vocabEncoder) {
   vector<string>::iterator piter = tgtLatticePaths.begin();
   for (; piter != tgtLatticePaths.end(); ++piter) {
      vector<int> tgtSent;
      fst::VectorFst<FstUtils::LogQuadArc> tgtFst;
      LoadLatticeFst(*piter, tgtSent, tgtFst, vocabEncoder);
      tgtSents.push_back(tgtSent);
      tgtFsts.push_back(tgtFst);
   }
}

void FstUtils::DecodePair(const FstUtils::LogPairWeight& w, float& v1, float& v2) {
    v1 = w.Value1().Value();
    v2 = w.Value2().Value();
}

void FstUtils::DecodeTriple(const FstUtils::LogTripleWeight& w, float& v1, float& v2, float& v3) {
    DecodePair(w.Value1(), v1, v2);
    v3 = w.Value2().Value();
}

void FstUtils::DecodeQuad(const FstUtils::LogQuadWeight& w, float& v1, float& v2, float& v3, float& v4) {
    DecodeTriple(w.Value1(), v1, v2, v3);
    v4 = w.Value2().Value();
}

void FstUtils::MakeOneFinalState(fst::VectorFst<LogQuadArc>& fst) {
    int finalStateId = fst.AddState();
    for(StateIterator< VectorFst<LogQuadArc> > siter(fst); !siter.Done(); siter.Next()) {
        FstUtils::LogQuadWeight stoppingWeight = fst.Final(siter.Value());
        if(stoppingWeight != FstUtils::LogQuadWeight::Zero()) {
            fst.SetFinal(siter.Value(), FstUtils::LogQuadWeight::Zero());
            fst.AddArc(siter.Value(), LogQuadArc(FstUtils::EPSILON, FstUtils::EPSILON, stoppingWeight, finalStateId));
        }
    }
    fst.SetFinal(finalStateId, FstUtils::LogQuadWeight::One());
}

void FstUtils::MakeOneFinalState(fst::VectorFst<FstUtils::LogArc>& fst) {
    int finalStateId = fst.AddState();
    for (StateIterator< VectorFst<FstUtils::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
        FstUtils::LogWeight stoppingWeight = fst.Final(siter.Value());
        if (stoppingWeight != FstUtils::LogWeight::Zero()) {
            fst.SetFinal(siter.Value(), FstUtils::LogWeight::Zero());
            fst.AddArc(siter.Value(), FstUtils::LogArc(FstUtils::EPSILON, FstUtils::EPSILON, stoppingWeight, finalStateId));
        }
    }
    fst.SetFinal(finalStateId, FstUtils::LogWeight::One());
}

// return the id of a final states in this fst. if no final state found, returns -1.
int FstUtils::FindFinalState(const fst::VectorFst<LogQuadArc>& fst) {
    for(StateIterator< VectorFst<LogQuadArc> > siter(fst); !siter.Done(); siter.Next()) {
        const LogQuadArc::StateId &stateId = siter.Value();
        if (fst.Final(stateId) != FstUtils::LogQuadWeight::Zero()) {
            return (int) stateId;
        }
    }
    return -1;
}

// return the id of a final states in this fst. if no final state found, returns -1.
int FstUtils::FindFinalState(const fst::VectorFst<FstUtils::LogArc>& fst) {
    for(StateIterator< VectorFst<FstUtils::LogArc> > siter(fst); !siter.Done(); siter.Next()) {
        const FstUtils::LogArc::StateId &stateId = siter.Value();
        if (fst.Final(stateId) != FstUtils::LogWeight::Zero()) {
            return (int) stateId;
        }
    }
    return -1;
}

bool FstUtils::AreShadowFsts(const fst::VectorFst<LogQuadArc>& fst1, \
                             const fst::VectorFst<FstUtils::LogArc>& fst2) {
    // verify number of states
    if(fst1.NumStates() != fst2.NumStates()) {
        cerr << "different state count" << endl;
        return false;
    }

    StateIterator< VectorFst<LogQuadArc> > siter1(fst1);
    StateIterator< VectorFst<FstUtils::LogArc> > siter2(fst2);
    while(!siter1.Done() || !siter2.Done()) {
        // verify state ids
        int from1 = siter1.Value(), from2 = siter2.Value();
        if  (from1 != from2) {
            cerr << "different state ids" << endl;
            return false;
        }
        ArcIterator< VectorFst<LogQuadArc> > aiter1(fst1, from1);
        ArcIterator< VectorFst<FstUtils::LogArc> > aiter2(fst2, from2);
        while(!aiter1.Done() || !aiter2.Done()) {
            // verify number of arcs leaving this state
            if (aiter1.Done() || aiter2.Done()) {
	            cerr << "different number of arcs leaving " << from1 << endl;
	            return false;
            }
            // verify the arc input/output labels
            if (aiter1.Value().ilabel != aiter2.Value().ilabel) {
	            cerr << "different input label" << endl;
	            return false;
            } else if (aiter1.Value().olabel != aiter2.Value().olabel) {
	            cerr << "different output label" << endl;
	            return false;
            }
            // verify the arc next state
            if (aiter1.Value().nextstate != aiter2.Value().nextstate) {
	            cerr << "different next states" << endl;
	            return false;
            }
            // advance the iterators
            aiter1.Next();
            aiter2.Next();
        }
        // advance hte iterators
        siter1.Next();
        siter2.Next();
    }
    return true;
}

// returns a moses-style alignment string compatible with the alignment represented in the transducer bestAlignment
// assumption:
// - bestAlignment is a linear chain transducer. 
// - the input labels are tgt positions
// - the output labels are the corresponding src positions according to the alignment
string FstUtils::PrintAlignment(const VectorFst<FstUtils::StdArc>& bestAlignment) {
    stringstream output;
    //  cerr << "best alignment FST summary: " << endl;
    //  cerr << PrintFstSummary<FstUtils::StdArc>(bestAlignment) << endl;

    // traverse the transducer beginning with the start state
    int startState = bestAlignment.Start();
    int currentState = startState;
    int tgtPos = 0;
    while(bestAlignment.Final(currentState) == FstUtils::LogWeight::Zero()) {
        // get hold of the arc
        ArcIterator< VectorFst< FstUtils::StdArc > > aiter(bestAlignment, currentState);
        // identify the next state
        int nextState = aiter.Value().nextstate;
        // skip epsilon arcs
        if(aiter.Value().ilabel == EPSILON && aiter.Value().olabel == EPSILON) {
            currentState = nextState;
            continue;
        }
        tgtPos++;
        if(aiter.Value().ilabel != tgtPos) {
            cerr << "aiter.Value().ilabel = " << aiter.Value().ilabel << ", whereas tgtPos = " << tgtPos << endl;
        }
        assert(aiter.Value().ilabel == tgtPos);

        assert(aiter.Value().olabel >= 0);

        int srcPos = aiter.Value().olabel;
        if (srcPos != 0) {
            output << (srcPos - 1) << "-" << (tgtPos - 1) << " ";
        }
        // this state shouldn't have other arcs!
        aiter.Next();
        assert(aiter.Done());
        // move forward to the next state
        currentState = nextState;
    }
    output << endl;
    return output.str();
}

// assumptions:
// - this fst is linear. no state has more than one outgoing/incoming arc
void FstUtils::LinearFstToVector(const fst::VectorFst<FstUtils::StdArc> &fst, \
                                 std::vector<int> &ilabels, std::vector<int> &olabels, \
                                 bool keepEpsilons) {
    assert(olabels.size() == 0);
    assert(ilabels.size() == 0);
    int currentState = fst.Start();
    do {
        ArcIterator< VectorFst<FstUtils::StdArc> > aiter(fst, currentState);
        if (keepEpsilons || aiter.Value().ilabel != EPSILON) {
            ilabels.push_back(aiter.Value().ilabel);
        }
        if (keepEpsilons || aiter.Value().olabel != EPSILON) {
            olabels.push_back(aiter.Value().olabel);
        }
        currentState = aiter.Value().nextstate;
        // assert it's a linear FST
        aiter.Next();
        assert(aiter.Done()); 
    } while(fst.Final(currentState) == FstUtils::TropicalWeight::Zero());
}

string FstUtils::PrintAlignmentWithLattice(const VectorFst< FstUtils::StdArc > &bestAlignment, VocabEncoder& vocabEncoder) {
    stringstream output;
    // traverse the transducer beginning with the start state
    int startState = bestAlignment.Start();
    int currentState = startState;
    int tgtPos = 0;
    string bestPath;
    while(bestAlignment.Final(currentState) == FstUtils::LogWeight::Zero()) {
        // get hold of the arc
        ArcIterator< VectorFst< FstUtils::StdArc > > aiter(bestAlignment, currentState);
        // identify the next state
        int nextState = aiter.Value().nextstate;
        // skip epsilon arcs
        if (aiter.Value().ilabel == EPSILON && aiter.Value().olabel == EPSILON) {
            currentState = nextState;
            continue;
        }
        int ilabel = aiter.Value().ilabel;
        string morpheme = vocabEncoder.Decode(ilabel);
        bestPath += morpheme + " ";
        tgtPos++;
        assert(aiter.Value().olabel >= 0);
        int srcPos = aiter.Value().olabel;
        if (srcPos != 0) {
            output << (srcPos - 1) << "-" << (tgtPos - 1) << " ";
        }
        aiter.Next();
        assert(aiter.Done());
        currentState = nextState;
    }
    output << endl;
    output << bestPath << endl;
    return output.str();
}
