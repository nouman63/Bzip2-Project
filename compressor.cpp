#include "compressor.h"
#include "rle1.h"
#include "bwt.h"
#include "huffman.h"
#include <fstream>
#include <iostream>
#include <chrono>

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

// RLE-2 encoding (run-length encoding with run lengths > 3)
// RLE-2 encoding/decoding
vector<unsigned char> Compressor::apply_rle2(const vector<unsigned char>& data, bool compress) {
    if (compress) {
        vector<unsigned char> output;
        size_t i = 0;

        while (i < data.size()) {
            size_t run_length = 1;
            while (i + run_length < data.size() &&
                data[i + run_length] == data[i] &&
                run_length < 255) {
                run_length++;
            }

            if (run_length >= 4) {
                // Encode run: [symbol, length]
                output.push_back(data[i]);
                output.push_back(static_cast<unsigned char>(run_length));
                i += run_length;
            }
            else {
                // Output as is
                for (size_t j = 0; j < run_length; j++) {
                    output.push_back(data[i + j]);
                }
                i += run_length;
            }
        }
        return output;
    }
    else {
        // RLE-2 Decoding
        vector<unsigned char> output;
        size_t i = 0;

        while (i < data.size()) {
            // Check if this is a run-length encoded pair
            // For simplicity, assume runs are encoded as [symbol, length] where length >= 4
            if (i + 1 < data.size()) {
                // Look ahead to see if next byte is a valid length (4-255)
                // This is a simplified approach
                unsigned char next = data[i + 1];
                if (next >= 4) {
                    // This is an encoded run
                    unsigned char symbol = data[i];
                    unsigned char length = next;
                    for (unsigned char j = 0; j < length; j++) {
                        output.push_back(symbol);
                    }
                    i += 2;
                    continue;
                }
            }
            // Not an encoded run, output as is
            output.push_back(data[i]);
            i++;
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

// Apply Huffman coding
vector<unsigned char> Compressor::apply_huffman(const vector<unsigned char>& data, bool compress) {
    if (compress) {
        auto result = huffman_encode(data);
        // Convert bits to bytes for storage
        vector<unsigned char> output;
        size_t bit_count = result.encoded_data.size();
        size_t byte_count = (bit_count + 7) / 8;
        output.resize(byte_count);
        
        for (size_t i = 0; i < bit_count; i++) {
            if (result.encoded_data[i]) {
                output[i / 8] |= (1 << (7 - (i % 8)));
            }
        }
        
        // TODO: Store Huffman tree for decoding
        return output;
    } else {
        // Decompress Huffman
        // TODO: Implement with tree reconstruction
        return data;
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

// Process a block through all transforms
vector<unsigned char> Compressor::process_block(const vector<unsigned char>& block, bool compress) {
    vector<unsigned char> data = block;
    
    if (compress) {
        // Compression pipeline (reverse order of decompression)
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
    } else {
        // Decompression pipeline (reverse order)
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
    }
    
    return data;
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
        cout << "  Block " << i << " compressed from " << block_data.size() 
             << " to " << compressed_block.size() << " bytes (Ratio: " 
             << (100.0 * compressed_block.size() / block_data.size()) << "%)" << endl;
    }
    
    output.close();
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "\nCompression complete!" << endl;
    cout << "Original size: " << original_size_ << " bytes" << endl;
    cout << "Compressed size: " << compressed_size_ << " bytes" << endl;
    cout << "Compression ratio: " << (100.0 * compressed_size_ / original_size_) << "%" << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;
    
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
    cout << "Original size: " << original_size_ << " bytes" << endl;
    cout << "Compressed size: " << compressed_size_ << " bytes" << endl;
    cout << "Space saved: " << (original_size_ - compressed_size_) << " bytes" << endl;
    cout << "Compression ratio: " << (100.0 * compressed_size_ / original_size_) << "%" << endl;
}