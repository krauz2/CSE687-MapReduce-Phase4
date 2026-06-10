#include "executive.h"  // We include Executive because main uses that class to start the program.

int main(int argc, char* argv[]) {
    Executive executive;                 // Create the Executive object that manages startup.
    return executive.run(argc, argv);   // Pass command-line info into Executive and return its result code.
}
