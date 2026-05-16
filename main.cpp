#include "compressor.h"
#include "block_management.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>

using namespace std;

static vector<unsigned char> read_file(const string& path) {
    ifstream f(path, ios::binary);
    return vector<unsigned char>((istreambuf_iterator<char>(f)), {});
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage:\n";
        cout << "  compress:   " << argv[0] << " -c <input> <output.bz2p>\n";
        cout << "  decompress: " << argv[0] << " -d <input.bz2p> <output>\n";
        cout << "  test:       " << argv[0] << " -t <input> <output.bz2p>\n";
        return 1;
    }

    CompressionConfig config;
    Compressor compressor(config);

    string mode  = argv[1];
    string arg2  = argv[2];
    string arg3  = argv[3];

    if (mode == "-c") {
        if (!compressor.compress(arg2, arg3)) {
            cerr << "Compression failed.\n";
            return 1;
        }
        compressor.print_stats();

    } else if (mode == "-d") {
        if (!compressor.decompress(arg2, arg3)) {
            cerr << "Decompression failed.\n";
            return 1;
        }

    } else if (mode == "-t") {
        // Round-trip: compress -> decompress -> compare with original
        string recovered = arg3 + ".recovered";

        cout << "--- Compressing " << arg2 << " ---\n";
        if (!compressor.compress(arg2, arg3)) {
            cerr << "Compression failed.\n";
            return 1;
        }
        compressor.print_stats();

        cout << "\n--- Decompressing " << arg3 << " ---\n";
        if (!compressor.decompress(arg3, recovered)) {
            cerr << "Decompression failed.\n";
            return 1;
        }

        auto orig = read_file(arg2);
        auto rec  = read_file(recovered);

        if (orig == rec) {
            cout << "\n Round-trip test PASSED (" << orig.size() << " bytes match)\n";
        } else {
            cout << "\n Round-trip test FAILED (original " << orig.size()
                 << " bytes, recovered " << rec.size() << " bytes)\n";
            return 1;
        }

    } else {
        cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}
