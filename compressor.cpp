#include "compressor.h"
#include "rle1.h"
#include "bwt.h"
#include "huffman.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <iterator>

using namespace std;

Compressor::Compressor(const CompressionConfig& config) 
    : config_(config), original_size_(0), compressed_size_(0) {}

// MTF encoding/decoding
vector<unsigned char> Compressor::apply_mtf(const vector<unsigned char>& data, bool compress) {
    if (data.empty()) return {};
    
    if (compress) {
        // MTF Encoding
        vector<unsigned char> output(data.size());
        vector<unsigned char> symbol_table(256);
        for (int i = 0; i < 256; i++) {
            symbol_table[i] = i;
        }
        
        for (size_t i = 0; i < data.size(); i++) {
            // Find symbol in table
            size_t pos = 0;
            while (pos < 256 && symbol_table[pos] != data[i]) {
                pos++;
            }
            
            output[i] = pos;
            
            // Move to front
            unsigned char temp = symbol_table[pos];
            for (size_t j = pos; j > 0; j--) {
                symbol_table[j] = symbol_table[j - 1];
            }
            symbol_table[0] = temp;
        }
        
        return output;
    } else {
        // MTF Decoding
        vector<unsigned char> output(data.size());
        vector<unsigned char> symbol_table(256);
        for (int i = 0; i < 256; i++) {
            symbol_table[i] = i;
        }
        
        for (size_t i = 0; i < data.size(); i++) {
            size_t pos = data[i];
            output[i] = symbol_table[pos];
            
            // Move to front
            unsigned char temp = symbol_table[pos];
            for (size_t j = pos; j > 0; j--) {
                symbol_table[j] = symbol_table[j - 1];
            }
            symbol_table[0] = temp;
        }
        
        return output;
    }
}

// RLE-2: compresses zero runs produced by MTF using an unambiguous 3-case scheme:
//   0x00 <count> : a run of 'count' zeros (count 1-255)
//   0x01 <byte>  : literal escape for bytes 0 and 255
//   byte+1       : for bytes 1-254 (stored as 2-255, never collides with tags)
vector<unsigned char> Compressor::apply_rle2(const vector<unsigned char>& data, bool compress) {
    if (compress) {
        vector<unsigned char> output;
        size_t i = 0;
        while (i < data.size()) {
            if (data[i] == 0) {
                size_t run = 0;
                while (i < data.size() && data[i] == 0 && run < 255) { run++; i++; }
                output.push_back(0x00);
                output.push_back((unsigned char)run);
            } else if (data[i] == 255) {
                output.push_back(0x01);
                output.push_back(255);
                i++;
            } else {
                output.push_back(data[i] + 1);  // maps 1-254 to 2-255
                i++;
            }
        }
        return output;
    } else {
        vector<unsigned char> output;
        size_t i = 0;
        while (i < data.size()) {
            unsigned char b = data[i++];
            if (b == 0x00) {
                if (i >= data.size()) break;
                unsigned char count = data[i++];
                for (unsigned char k = 0; k < count; k++) output.push_back(0);
            } else if (b == 0x01) {
                if (i >= data.size()) break;
                output.push_back(data[i++]);
            } else {
                output.push_back(b - 1);  // maps 2-255 back to 1-254
            }
        }
        return output;
    }
}
// Apply BWT transform
vector<unsigned char> Compressor::apply_bwt(const vector<unsigned char>& data, bool compress) {
    if (compress) {
        auto result = bwt_encode(data);
        // Store primary index with the data (e.g., as first 4 bytes)
        vector<unsigned char> output(4 + result.data.size());
        output[0] = (result.primary_index >> 24) & 0xFF;
        output[1] = (result.primary_index >> 16) & 0xFF;
        output[2] = (result.primary_index >> 8) & 0xFF;
        output[3] = result.primary_index & 0xFF;
        copy(result.data.begin(), result.data.end(), output.begin() + 4);
        return output;
    } else {
        // Extract primary index and data
        if (data.size() < 4) return {};
        int primary_index = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        vector<unsigned char> encoded_data(data.begin() + 4, data.end());
        return bwt_decode(encoded_data, primary_index);
    }
}

// Apply Huffman coding using canonical Huffman with self-contained serialization.
vector<unsigned char> Compressor::apply_huffman(const vector<unsigned char>& data, bool compress) {
    if (compress) {
        return huffman_compress(data);
    } else {
        return huffman_decompress(data);
    }
}

// Apply RLE-1
vector<unsigned char> Compressor::apply_rle1(const vector<unsigned char>& data, bool compress) {
    if (compress) {
        return rle1_encode(data);
    } else {
        return rle1_decode(data);
    }
}

// Process a block through all transforms.
// Flag byte is prepended: 0x01 = compressed, 0x00 = stored raw (when compression didn't help).
vector<unsigned char> Compressor::process_block(const vector<unsigned char>& block, bool compress) {
    vector<unsigned char> data = block;

    if (compress) {
        if (config_.rle1_enabled) {
            data = apply_rle1(data, true);
            cout << "  After RLE-1: " << data.size() << " bytes" << endl;
        }
        if (config_.bwt_type == "matrix") {
            data = apply_bwt(data, true);
            cout << "  After BWT: " << data.size() << " bytes" << endl;
        }
        if (config_.mtf_enabled) {
            data = apply_mtf(data, true);
            cout << "  After MTF: " << data.size() << " bytes" << endl;
        }
        if (config_.rle2_enabled) {
            data = apply_rle2(data, true);
            cout << "  After RLE-2: " << data.size() << " bytes" << endl;
        }
        if (config_.huffman_enabled) {
            data = apply_huffman(data, true);
            cout << "  After Huffman: " << data.size() << " bytes" << endl;
        }

        // If pipeline made the block larger, store original data raw (with flag 0x00).
        // This prevents small files or already-compressed data from expanding.
        vector<unsigned char> result;
        if (data.size() < block.size()) {
            result.reserve(1 + data.size());
            result.push_back(0x01);  // compressed
            result.insert(result.end(), data.begin(), data.end());
            cout << "  Stored compressed." << endl;
        } else {
            result.reserve(1 + block.size());
            result.push_back(0x00);  // raw passthrough
            result.insert(result.end(), block.begin(), block.end());
            cout << "  Stored raw (compression did not help for this block)." << endl;
        }
        return result;

    } else {
        // Read flag byte first
        if (data.empty()) return {};
        unsigned char flag = data[0];
        data = vector<unsigned char>(data.begin() + 1, data.end());

        if (flag == 0x00) {
            // Raw passthrough — return as-is
            return data;
        }

        // flag == 0x01: run full decompression pipeline
        if (config_.huffman_enabled) {
            data = apply_huffman(data, false);
            cout << "  After Huffman decode: " << data.size() << " bytes" << endl;
        }
        if (config_.rle2_enabled) {
            data = apply_rle2(data, false);
            cout << "  After RLE-2 decode: " << data.size() << " bytes" << endl;
        }
        if (config_.mtf_enabled) {
            data = apply_mtf(data, false);
            cout << "  After MTF decode: " << data.size() << " bytes" << endl;
        }
        if (config_.bwt_type == "matrix") {
            data = apply_bwt(data, false);
            cout << "  After BWT decode: " << data.size() << " bytes" << endl;
        }
        if (config_.rle1_enabled) {
            data = apply_rle1(data, false);
            cout << "  After RLE-1 decode: " << data.size() << " bytes" << endl;
        }
        return data;
    }
}

// Compress a file
bool Compressor::compress(const string& input_file, const string& output_file) {
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "\nCompressing: " << input_file << endl;
    
    // Divide into blocks
    BlockManager manager = divide_into_blocks(input_file, config_.block_size);
    if (manager.blocks.empty()) {
        return false;
    }
    
    // Calculate original size
    original_size_ = 0;
    for (const auto& block : manager.blocks) {
        original_size_ += block.size;
    }
    cout << "Original size: " << original_size_ << " bytes" << endl;
    
    // Process each block
    ofstream output(output_file, ios::binary);
    if (!output.is_open()) {
        cerr << "Error opening output file: " << output_file << endl;
        return false;
    }
    
    // Write number of blocks
    uint32_t num_blocks = manager.blocks.size();
    output.write(reinterpret_cast<char*>(&num_blocks), sizeof(num_blocks));
    
    compressed_size_ = 0;
    for (size_t i = 0; i < manager.blocks.size(); i++) {
        cout << "\nProcessing block " << i + 1 << "/" << manager.blocks.size() << endl;
        
        vector<unsigned char> block_data(manager.blocks[i].data, 
                                         manager.blocks[i].data + manager.blocks[i].size);
        
        // Compress the block
        auto compressed_block = process_block(block_data, true);
        
        // Write block size and compressed data
        uint32_t block_size = compressed_block.size();
        output.write(reinterpret_cast<char*>(&block_size), sizeof(block_size));
        output.write(reinterpret_cast<char*>(compressed_block.data()), block_size);
        
        compressed_size_ += block_size;
        double ratio = (double)original_size_ > 0
            ? (double)compressed_block.size() / block_data.size() : 1.0;
        cout << "  Block " << i << ": " << block_data.size()
             << " -> " << compressed_block.size() << " bytes"
             << " (" << fixed << setprecision(2) << ratio << "x ratio)" << endl;
    }
    
    output.close();
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "\nCompression complete!" << endl;
    cout << "Original size:   " << original_size_ << " bytes" << endl;
    cout << "Compressed size: " << compressed_size_ << " bytes" << endl;
    long long saved = (long long)original_size_ - (long long)compressed_size_;
    if (saved >= 0)
        cout << "Space saved:     " << saved << " bytes ("
             << fixed << setprecision(2) << (100.0 * saved / original_size_) << "%)" << endl;
    else
        cout << "Size increased:  " << -saved << " bytes (file too small to compress)" << endl;
    cout << "Time taken:      " << duration.count() << " ms" << endl;
    
    return true;
}

// Decompress a file
bool Compressor::decompress(const string& input_file, const string& output_file) {
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "\nDecompressing: " << input_file << endl;
    
    ifstream input(input_file, ios::binary);
    if (!input.is_open()) {
        cerr << "Error opening input file: " << input_file << endl;
        return false;
    }
    
    // Read number of blocks
    uint32_t num_blocks;
    input.read(reinterpret_cast<char*>(&num_blocks), sizeof(num_blocks));
    
    BlockManager manager;
    manager.block_size = config_.block_size;
    manager.blocks.reserve(num_blocks);
    
    size_t decompressed_size = 0;
    
    // Process each block
    for (uint32_t i = 0; i < num_blocks; i++) {
        cout << "\nProcessing block " << i + 1 << "/" << num_blocks << endl;
        
        // Read compressed block size
        uint32_t compressed_block_size;
        input.read(reinterpret_cast<char*>(&compressed_block_size), sizeof(compressed_block_size));
        
        // Read compressed block data
        vector<unsigned char> compressed_block(compressed_block_size);
        input.read(reinterpret_cast<char*>(compressed_block.data()), compressed_block_size);
        
        // Decompress the block
        auto decompressed_block = process_block(compressed_block, false);
        
        // Create block
        Block block;
        block.original_size = decompressed_block.size();
        block.size = decompressed_block.size();
        block.data = new unsigned char[block.size];
        copy(decompressed_block.begin(), decompressed_block.end(), block.data);
        
        manager.blocks.push_back(move(block));
        decompressed_size += decompressed_block.size();
        
        cout << "  Block " << i << " decompressed to " << decompressed_block.size() << " bytes" << endl;
    }
    
    input.close();
    
    // Reassemble blocks
    if (!reassemble_blocks(manager, output_file)) {
        return false;
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "\nDecompression complete!" << endl;
    cout << "Decompressed size: " << decompressed_size << " bytes" << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;
    
    return true;
}

void Compressor::print_stats() const {
    cout << "\n=== Compression Statistics ===" << endl;
    cout << "Original size:   " << original_size_ << " bytes" << endl;
    cout << "Compressed size: " << compressed_size_ << " bytes" << endl;
    long long saved = (long long)original_size_ - (long long)compressed_size_;
    if (saved >= 0)
        cout << "Space saved:     " << saved << " bytes ("
             << fixed << setprecision(2) << (100.0 * saved / original_size_) << "%)" << endl;
    else
        cout << "Size increased:  " << -saved << " bytes (file too small to compress)" << endl;
    if (compressed_size_ > 0)
        cout << "Compression ratio: " << fixed << setprecision(2)
             << (double)original_size_ / compressed_size_ << ":1" << endl;
}