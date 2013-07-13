#include <iostream>

#include "LearningInfo.h"
#include "FstUtils.h"
#include "StringUtils.h"
#include "IbmModel1.h"

using namespace fst;
using namespace std;

typedef ProductArc<LogWeight, LogWeight> ProductLogArc;

void print_usage() {
    cout << "train-model1 bitext output_prefix use_lattice" << endl;
}

void ParseParameters(int argc, char **argv, string& bitextCorpusFilename, string &outputFilepathPrefix, bool& use_lattice) {
    if (argc != 4) {
        print_usage();
    } else {
        bitextCorpusFilename = argv[1];
        outputFilepathPrefix = argv[2];
        use_lattice = atoi(argv[3]) == 0 ? false : true; 
    }
}

int main(int argc, char **argv) {
    // parse arguments
    cout << "parsing arguments" << endl;
    bool use_lattice;
    string bitextCorpusFilename, outputFilenamePrefix;
    ParseParameters(argc, argv, bitextCorpusFilename, outputFilenamePrefix, use_lattice);
    
    // specify stopping criteria
    LearningInfo learningInfo;
    learningInfo.maxIterationsCount = 20;
    learningInfo.useMaxIterationsCount = true;
    //  learningInfo.useEarlyStopping = true;
    learningInfo.minLikelihoodDiff = 100.0;
    learningInfo.useMinLikelihoodDiff = false;
    learningInfo.minLikelihoodRelativeDiff = 0.01;
    learningInfo.useMinLikelihoodRelativeDiff = true;

    // initialize the model
    IbmModel1 model(bitextCorpusFilename, outputFilenamePrefix, learningInfo, use_lattice);

    // train model parameters
    if (use_lattice) {
        model.TrainWithLattices();
    } else {
        model.Train();
    }
}
