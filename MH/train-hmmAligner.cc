#include <cstdlib>
#include <map>

#include "LearningInfo.h"
#include "FstUtils.h"
#include "StringUtils.h"
#include "HmmAligner.h"
#include "IbmModel1.h"
 
using namespace fst;
using namespace std;
using namespace boost;

bool print_usage() {
   cout << "training: ./train-hmmAligner 1 bitextFilename outputFilePathPrefix tParameters use_lattice" << endl;
   cout << "decoding: ./train-hmmAligner 2 tParameters aParameters testFilename outputFilePathPrefix" << endl;
}

bool ParseParameters(int argc, char **argv, string& bitextFilename,  \
                     string& outputFilepathPrefix, \
                     string& tParams, string &aParams, \
                     string& testFilename, int& mode, \
                     bool& use_lattice) {
   if (argc != 6) {
       print_usage();
       return false;
   }
   else {
       mode = atoi(argv[1]);
       if (mode == 1) {
           if (argc != 6) {
               print_usage();
               return false;
           }
           else {
               bitextFilename = argv[2];
               outputFilepathPrefix = argv[3];
               tParams = argv[4];
               use_lattice = atoi(argv[5]) == 0 ? false : true;
               return true;
           }
       }
       else if (mode == 2) {
           if (argc != 6) {
               print_usage();
               return false;
           }
           else {
               tParams = argv[2];
               aParams = argv[3];
               testFilename = argv[4];
               outputFilepathPrefix = argv[5];
               return true;
           }
       }
       else {
           print_usage();
           return false;
       }
   }
}

int main(int argc, char **argv) {

    mpi::environment env(argc, argv);
    mpi::communicator world;
    // parse arguments
    cout << "parsing arguments" << endl;
    int mode;
    bool use_lattice;
    string bitextFilename, tParams, aParams, outputFilenamePrefix, testFilename;
    if (ParseParameters(argc, argv, \
                        bitextFilename, outputFilenamePrefix, \
                        tParams, aParams, testFilename, mode, use_lattice)) {
        // specify stopping criteria
        LearningInfo learningInfo;
        learningInfo.maxIterationsCount = 100;
        learningInfo.useMaxIterationsCount = true;
        learningInfo.minLikelihoodDiff = 100.0;
        learningInfo.useMinLikelihoodDiff = false;
        learningInfo.minLikelihoodRelativeDiff = 0.01;
        learningInfo.useMinLikelihoodRelativeDiff = true;
        learningInfo.debugLevel = DebugLevel::CORPUS;
        learningInfo.useEarlyStopping = false;
        learningInfo.persistParamsAfterNIteration = 1;
        learningInfo.persistFinalParams = true;
        learningInfo.smoothMultinomialParams = false; 
        learningInfo.mpiWorld = &world;

        if (mode == 1) { 
            // initialize the model
            HmmAligner model(bitextFilename, outputFilenamePrefix, learningInfo, tParams, use_lattice);

            // train model parameters
            if (use_lattice) {
                model.TrainWithLattices();
                // align the test set
                string outputAlignmentsFilename = outputFilenamePrefix + ".train.align";
                model.AlignWithLattice(outputAlignmentsFilename);
            }
            else {
                model.Train();
                string outputAlignmentsFilename = outputFilenamePrefix + ".train.align";
                model.Align(outputAlignmentsFilename);
            }

        }
        else {
            HmmAligner model(tParams, aParams, testFilename);
            string outputAlignmentsFilename = outputFilenamePrefix + ".align";
            model.AlignWithLattice(outputAlignmentsFilename);
        }
        return 0;
    }
    else {
        return - 1;
    }
}

