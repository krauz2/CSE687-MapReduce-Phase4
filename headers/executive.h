#pragma once

class Executive {
public:
    // Phase 4 controller arguments:
    //   Phase4.exe <input_directory> <dll_directory> [output_directory] [temp_directory] [reducer_count]
    int run(int argc, char* argv[]);
};
