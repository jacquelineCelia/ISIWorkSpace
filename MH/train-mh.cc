#include <iostream>
#include <cstdlib>

#include "mhhmm.h"
#include "mhibm.h"

using namespace std;

void usage() {
    cerr << "./train-mh 0[ibm training] bitext basename\n"
         << "./train-mh 1[hmm training] bitext basename tparams\n" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4 && argc != 5) {
        usage();
        return -1;
    }
    int mode = atoi(argv[1]);
    if (mode == 0) {
        string bitext = argv[2];
        string basename = argv[3];
        MHIBM ibm(bitext, basename);
        ibm.Train(1000);
    }
    else if (mode == 1) {
        string bitext = argv[2];
        string basename = argv[3];
        string tparams = argv[4];
        MHHMM hmm(bitext, basename);
        hmm.Train(1000, tparams);
    }
    else {
        cerr << "Unsupported training mode. Accept [0:ibm model 1 / 1:hmm model]." << endl;
    }

    return 0;
}
