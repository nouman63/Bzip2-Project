#pragma once
#ifndef BLOCK_MANAGEMENT_H
#define BLOCK_MANAGEMENT_H

#include <cstddef>
#include <string>
#include <vector>

using namespace std;

// Structure to hold a single block of data
struct Block {
    unsigned char* data;        // Pointer to block data
    size_t size;                // Current size of block
    size_t original_size;       // Original size before compression

    Block() : data(nullptr), size(0), original_size(0) {}
    ~Block() { delete[] data; }

    // Disable copy
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    // Enable move
    Block(Block&& other) noexcept
        : data(other.data), size(other.size), original_size(other.original_size) {
        other.data = nullptr;
        other.size = 0;
        other.original_size = 0;
    }

    Block& operator=(Block&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            original_size = other.original_size;
            other.data = nullptr;
            other.size = 0;
            other.original_size = 0;
        }
        return *this;
    }
};

// Structure to manage multiple blocks
struct BlockManager {
    vector<Block> blocks;
    size_t block_size;          // Configurable block size

    BlockManager() : block_size(500 * 1024) {}
};

// Configuration structure for all compression settings
struct CompressionConfig {
    // General settings
    size_t block_size;          // Block size in bytes (100KB-900KB)
    bool rle1_enabled;          // Enable RLE-1 encoding
    string bwt_type;            // "matrix" or "suffix_array"
    bool mtf_enabled;           // Enable Move-To-Front transform
    bool rle2_enabled;          // Enable RLE-2 encoding
    bool huffman_enabled;       // Enable Huffman coding

    // Performance settings
    bool benchmark_mode;        // Enable benchmark mode
    bool output_metrics;        // Output compression metrics

    // Paths
    string input_directory;     // Input directory path

    CompressionConfig()
        : block_size(500 * 1024),
        rle1_enabled(true),
        bwt_type("matrix"),
        mtf_enabled(true),
        rle2_enabled(true),
        huffman_enabled(true),
        benchmark_mode(false),
        output_metrics(true),
        input_directory("./benchmarks/") {
    }
};

// Function prototypes
BlockManager divide_into_blocks(const string& filename, size_t block_size);
bool reassemble_blocks(const BlockManager& manager, const string& output_filename);
bool load_configuration(const string& config_file, CompressionConfig& config);
void print_configuration(const CompressionConfig& config);

#endif // BLOCK_MANAGEMENT_H