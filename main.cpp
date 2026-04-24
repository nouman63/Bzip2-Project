#include "block_management.h"
#include "compressor.h"
#include <iostream>
#include <string>
using namespace std;

int main() {
    int choice;
    string input_file, output_file;

    cout << "\n=== Data Compression Program ===\n";
    cout << "1. Compress a file\n";
    cout << "2. Decompress a file\n";
    cout << "3. Exit\n";
    cout << "Enter your choice: ";
    cin >> choice;
    cin.ignore();

    if (choice == 1) {
        cout << "\nEnter input filename: ";
        getline(cin, input_file);
        cout << "Enter output filename: ";
        getline(cin, output_file);

        // Use default configuration
        CompressionConfig config;
        config.block_size = 500 * 1024;
        config.rle1_enabled = true;
        config.bwt_type = "matrix";
        config.mtf_enabled = true;
        config.rle2_enabled = true;
        config.huffman_enabled = true;
        config.output_metrics = true;

        Compressor compressor(config);

        cout << "\nCompressing..." << endl;
        if (compressor.compress(input_file, output_file)) {
            cout << "\n✓ Compression successful!" << endl;
        }
        else {
            cout << "\n✗ Compression failed!" << endl;
        }
    }
    else if (choice == 2) {
        cout << "\nEnter input filename (compressed file): ";
        getline(cin, input_file);
        cout << "Enter output filename (decompressed file): ";
        getline(cin, output_file);

        CompressionConfig config;
        config.block_size = 500 * 1024;
        config.rle1_enabled = true;
        config.bwt_type = "matrix";
        config.mtf_enabled = true;
        config.rle2_enabled = true;
        config.huffman_enabled = true;

        Compressor compressor(config);

        cout << "\nDecompressing..." << endl;
        if (compressor.decompress(input_file, output_file)) {
            cout << "\n✓ Decompression successful!" << endl;
        }
        else {
            cout << "\n✗ Decompression failed!" << endl;
        }
    }
    else {
        cout << "Goodbye!" << endl;
    }

    cout << "\nPress Enter to exit...";
    cin.get();
    return 0;
}