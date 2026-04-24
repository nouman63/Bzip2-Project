#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "block_management.h"
#include <vector>
#include <string>

using namespace std;

// Main compressor class
class Compressor {
public:
    explicit Compressor(const CompressionConfig& config);

    // Compress a file
    bool compress(const string& input_file, const string& output_file);

    // Decompress a file
    bool decompress(const string& input_file, const string& output_file);

    // Get compression statistics
    void print_stats() const;

private:
    CompressionConfig config_;
    size_t original_size_;
    size_t compressed_size_;

    // Process a single block through all enabled transforms
    vector<unsigned char> process_block(const vector<unsigned char>& block, bool compress);

    // Individual transforms
    vector<unsigned char> apply_rle1(const vector<unsigned char>& data, bool compress);
    vector<unsigned char> apply_bwt(const vector<unsigned char>& data, bool compress);
    vector<unsigned char> apply_mtf(const vector<unsigned char>& data, bool compress);
    vector<unsigned char> apply_rle2(const vector<unsigned char>& data, bool compress);
    vector<unsigned char> apply_huffman(const vector<unsigned char>& data, bool compress);
};

#endif // COMPRESSOR_H#pragma once
