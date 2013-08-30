#ifndef _FST_UTILS_H_	  
#define _FST_UTILS_H_

#include <ctime>
#include <cstdlib>
#include <typeinfo>

#include <fst/fstlib.h>
#include <fst/weight.h>
#include <fst/util.h>

#include "VocabEncoder.h"

class FstUtils {
 public:
    static const int LOG_ZERO = 30;
    static const int EPSILON = 0;
    static const float LOG_PROBS_MUST_BE_GREATER_THAN_ME; // set in FstUtils.cc
  
    inline static float nLog(double prob) {
        return -1.0 * log(prob);
    }
  
    inline static double nExp(float exponent) {
        return exp(-1.0 * exponent);
    }
  
    // high precision LogWeight
    typedef fst::LogWeightTpl<double> LogWeight;
    typedef fst::TropicalWeightTpl<double> TropicalWeight;
  
    // high precision StdArc
    struct StdArc {
        typedef TropicalWeight Weight;
        typedef int Label;
        typedef int StateId;
    
        static const std::string &Type() {return type;}
        StdArc(Label ilabel, Label olabel, Weight weight, StateId nextstate) : \
            ilabel(ilabel), olabel(olabel), weight(weight), nextstate(nextstate) { }
        StdArc() { 
            ilabel = olabel = nextstate = 0; weight = Weight::Zero(); 
        }
    
        const static std::string type;
        Label ilabel;
        Label olabel;
        Weight weight;
        StateId nextstate;
    };
  
    // high precision LogArc
    struct LogArc {
        typedef LogWeight Weight;
        typedef int Label;
        typedef int StateId;

        static const std::string &Type() {return LogArc::type;}
        LogArc(Label ilabel, Label olabel, Weight weight, StateId nextstate) : \
             ilabel(ilabel), olabel(olabel), weight(weight), nextstate(nextstate) { }
        LogArc() {
             ilabel = olabel = nextstate = 0; weight = Weight::Zero(); 
        }
        ~LogArc() { }

        const static std::string type;
        Label ilabel;
        Label olabel;
        Weight weight;
        StateId nextstate;
    };
  
    // pair typedef
    typedef fst::ProductWeight<LogWeight, LogWeight> LogPairWeight;
    typedef fst::ProductArc<LogWeight, LogWeight> LogPairArc;
  
    // triple typedef
    typedef fst::ProductWeight<LogPairWeight, LogWeight> LogTripleWeight;
    typedef fst::ProductArc<LogPairWeight, LogWeight> LogTripleArc;
  
    // quadruple typedef
    typedef fst::ProductWeight<LogTripleWeight, LogWeight> LogQuadWeight;
    typedef fst::ProductArc<LogTripleWeight, LogWeight> LogQuadArc;
  
    static void LinearFstToVector(const fst::VectorFst<FstUtils::StdArc> &fst, \
                                  std::vector<int> &ilabels, \
                                  std::vector<int> &olables, \
                                  bool keepEpsilons = false);
  
    static bool AreShadowFsts(const fst::VectorFst<FstUtils::LogQuadArc>& fst1, \
                              const fst::VectorFst<FstUtils::LogArc>& fst2);

    static int FindFinalState(const fst::VectorFst<FstUtils::LogQuadArc>& fst);
    static int FindFinalState(const fst::VectorFst<FstUtils::LogArc>& fst);

    static void MakeOneFinalState(fst::VectorFst<FstUtils::LogArc>& fst);
    static void MakeOneFinalState(fst::VectorFst<FstUtils::LogQuadArc>& fst);
  
    static LogPairWeight EncodePair(float val1, float val2); 
    static LogTripleWeight EncodeTriple(float val1, float val2, float val3);
    static LogQuadWeight EncodeQuad(float val1, float val2, float val3, float val4);

    static LogPairWeight EncodePairInfinity();
    static LogTripleWeight EncodeTripleInfinity();
    static LogQuadWeight EncodeQuadInfinity();

    static void DecodePair(const LogPairWeight& w, float& v1, float& v2);
    static void DecodeTriple(const LogTripleWeight& w, float& v1, float& v2, float& v3);
    static void DecodeQuad(const LogQuadWeight& w, float& v1, float& v2, float& v3, float& v4);

    static string PrintAlignment(const fst::VectorFst< FstUtils::StdArc > &bestAlignment);
    static string PrintAlignmentWithLattice( \
        const fst::VectorFst<FstUtils::StdArc> &bestAlignment, VocabEncoder& vocabEncoder);

    static string PrintWeight(const TropicalWeight& w);
    static string PrintWeight(const LogWeight& w);
    static string PrintWeight(const LogPairWeight& w);
    static string PrintWeight(const LogTripleWeight& w);
    static string PrintWeight(const LogQuadWeight& w);
   
    static void LoadLatticeFsts(vector<string>& tgtLatticePaths, \
                                vector<vector<int> >& tgtSents, \
                                vector<fst::VectorFst<FstUtils::LogQuadArc> >& tgtFsts, \
                                VocabEncoder& vocabEncoder, 
                                bool use_weight, const float K = 1);

    static void LoadLatticeFst(const string tgtLatticePath, \
                               vector<int>& tgtSent, \
                               fst::VectorFst<FstUtils::LogQuadArc>& tgtFst, \
                               VocabEncoder& vocabEncoder, bool use_weight, \
                               const float K = 1);

    static void LoadLatticeFsts(vector<string>& tgtLatticePaths, \
                                vector<vector<int> >& tgtSents, \
                                vector<fst::VectorFst<FstUtils::LogArc> >& tgtFsts, \
                                VocabEncoder& vocabEncoder);

    static void LoadLatticeFst(const string tgtLatticePath, \
                               vector<int>& tgtSent, \
                               fst::VectorFst<FstUtils::LogArc>& tgtFst, \
                               VocabEncoder& vocabEncoder);

    template<class ArcType>
        inline static string PrintFstSummary(const fst::VectorFst<ArcType>& fst) {
            std::stringstream ss;
        ss << "=======" << endl;
        ss << "states:" << endl;
        ss << "=======" << endl << endl;
        for(fst::StateIterator<fst::VectorFst<ArcType> > siter(fst); \
            !siter.Done(); siter.Next()) {
            const int &stateId = siter.Value();
            string initial = fst.Start() == stateId? " START " : "";
            ss << "state:" << stateId << initial 
               << " FinalScore=" <<  PrintWeight(fst.Final(stateId)) << endl;
            ss << "arcs:" << endl;
            for(fst::ArcIterator< fst::VectorFst<ArcType> > aiter(fst, stateId); \
                    !aiter.Done(); aiter.Next()) {
                const ArcType &arc = aiter.Value();
                ss << arc.ilabel << ":" << arc.olabel << " " <<  stateId;
                ss << "-->" << arc.nextstate << " " << PrintWeight(arc.weight) << endl;
            } 
            ss << endl;
        }
        return ss.str(); 
    }

    struct LogTripleToLogMapper {
        FstUtils::LogArc operator()(const FstUtils::LogTripleArc &arc) const {
            float v1, v2, v3;
            FstUtils::DecodeTriple(arc.weight, v1, v2, v3);
            return FstUtils::LogArc(arc.ilabel, arc.olabel, v3, arc.nextstate);
        }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };

    struct LogTripleToLogPositionMapper {
        FstUtils::LogArc operator()(const FstUtils::LogTripleArc &arc) const {
            float v1, v2, v3;
            FstUtils::DecodeTriple(arc.weight, v1, v2, v3);
            return FstUtils::LogArc(arc.ilabel, arc.olabel, v3, arc.nextstate);
        }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };

    struct LogQuadToLogMapper {
        FstUtils::LogArc operator()(const FstUtils::LogQuadArc &arc) const {
            float v1, v2, v3, v4;
            FstUtils::DecodeQuad(arc.weight, v1, v2, v3, v4);
            return FstUtils::LogArc(arc.ilabel, arc.olabel, v4, arc.nextstate);
        }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };

    struct LogQuadToLogPositionMapper {
        FstUtils::LogArc operator()(const FstUtils::LogQuadArc &arc) const {
            float tgtPos, srcPos, v3, logprob;
            FstUtils::DecodeQuad(arc.weight, tgtPos, srcPos, v3, logprob);
            if  (arc.ilabel == FstUtils::EPSILON && arc.olabel == FstUtils::EPSILON) {
                return FstUtils::LogArc(FstUtils::EPSILON, \
                                        FstUtils::EPSILON, \
                                        logprob, \
                                        arc.nextstate);
            }
            return FstUtils::LogArc((int) tgtPos, (int) srcPos, logprob, arc.nextstate);
        }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };


    struct LogQuadToLogPositionMapperLattice {
        FstUtils::LogArc operator()(const FstUtils::LogQuadArc &arc) const {
            float tgtPos, srcPos, v3, logprob;
            FstUtils::DecodeQuad(arc.weight, tgtPos, srcPos, v3, logprob);
            if (arc.ilabel == FstUtils::EPSILON && arc.olabel == FstUtils::EPSILON) {
                return FstUtils::LogArc(FstUtils::EPSILON, \
                                        FstUtils::EPSILON, \
                                        logprob, arc.nextstate);
                }
            return FstUtils::LogArc(arc.ilabel, (int) srcPos, logprob, arc.nextstate);
        }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };

    struct LogToTropicalMapper {
        FstUtils::StdArc operator()(const FstUtils::LogArc &arc) const {
            return FstUtils::StdArc(arc.ilabel, \
                                    arc.olabel, \
                                    arc.weight.Value(), arc.nextstate);
        }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };

    struct TropicalToLogMapper {
        FstUtils::LogArc operator()(const FstUtils::StdArc &arc) const {
            return FstUtils::LogArc(arc.ilabel, \
                                    arc.olabel, \
                                    arc.weight.Value(), arc.nextstate);
            }
        fst::MapFinalAction FinalAction() const {return fst::MAP_NO_SUPERFINAL;}
        fst::MapSymbolsAction InputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        fst::MapSymbolsAction OutputSymbolsAction() const {return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const {return props;}
    };
};

#endif
