#include "block_management.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

// Trim whitespace from string
static string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// Parse INI configuration file
bool load_configuration(const string& config_file, CompressionConfig& config) {
    ifstream file(config_file);
    if (!file.is_open()) {
        cerr << "Error opening configuration file: " << config_file << endl;
        return false;
    }

    string line;
    string current_section;

    while (getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = trim(line);
        if (line.empty()) continue;

        // Check for section header
        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }

        // Parse key-value pair
        size_t equals_pos = line.find('=');
        if (equals_pos != string::npos) {
            string key = trim(line.substr(0, equals_pos));
            string value = trim(line.substr(equals_pos + 1));

            if (current_section == "General") {
                if (key == "block_size") {
                    size_t size_bytes = stoul(value);
                    if (size_bytes < 100 * 1024 || size_bytes > 900 * 1024) {
                        cerr << "Warning: Block size " << size_bytes
                            << " bytes out of range (100KB-900KB). Using default." << endl;
                    }
                    else {
                        config.block_size = size_bytes;
                    }
                }
                else if (key == "rle1_enabled") {
                    config.rle1_enabled = (value == "true" || value == "1");
                }
                else if (key == "bwt_type") {
                    config.bwt_type = value;
                }
                else if (key == "mtf_enabled") {
                    config.mtf_enabled = (value == "true" || value == "1");
                }
                else if (key == "rle2_enabled") {
                    config.rle2_enabled = (value == "true" || value == "1");
                }
                else if (key == "huffman_enabled") {
                    config.huffman_enabled = (value == "true" || value == "1");
                }
            }
            else if (current_section == "Performance") {
                if (key == "benchmark_mode") {
                    config.benchmark_mode = (value == "true" || value == "1");
                }
                else if (key == "output_metrics") {
                    config.output_metrics = (value == "true" || value == "1");
                }
            }
            else if (current_section == "Paths") {
                if (key == "input_directory") {
                    config.input_directory = value;
                }
            }
        }
    }

    return true;
}

// Print configuration
void print_configuration(const CompressionConfig& config) {
    cout << "\n=== Compression Configuration ===" << endl;
    cout << "[General]" << endl;
    cout << "  block_size = " << config.block_size << " bytes ("
        << config.block_size / 1024.0 << " KB)" << endl;
    cout << "  rle1_enabled = " << (config.rle1_enabled ? "true" : "false") << endl;
    cout << "  bwt_type = " << config.bwt_type << endl;
    cout << "  mtf_enabled = " << (config.mtf_enabled ? "true" : "false") << endl;
    cout << "  rle2_enabled = " << (config.rle2_enabled ? "true" : "false") << endl;
    cout << "  huffman_enabled = " << (config.huffman_enabled ? "true" : "false") << endl;
    cout << "[Performance]" << endl;
    cout << "  benchmark_mode = " << (config.benchmark_mode ? "true" : "false") << endl;
    cout << "  output_metrics = " << (config.output_metrics ? "true" : "false") << endl;
    cout << "[Paths]" << endl;
    cout << "  input_directory = " << config.input_directory << endl;
    cout << "================================" << endl;
}

// Reads input file and divides into blocks
BlockManager divide_into_blocks(const string& filename, size_t block_size) {
    BlockManager manager;
    manager.block_size = block_size;

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening input file: " << filename << endl;
        return manager;
    }

    // Get file size
    file.seekg(0, ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, ios::beg);

    if (file_size == 0) {
        cerr << "Error: File is empty" << endl;
        return manager;
    }

    // Calculate number of blocks needed
    size_t num_blocks = (file_size + block_size - 1) / block_size;
    manager.blocks.reserve(num_blocks);

    // Read file and divide into blocks
    for (size_t i = 0; i < num_blocks; i++) {
        // Calculate size for this block
        size_t current_block_size = min(block_size, file_size - i * block_size);

        Block block;
        block.original_size = current_block_size;
        block.size = current_block_size;
        block.data = new unsigned char[current_block_size];

        // Read block data
        file.read(reinterpret_cast<char*>(block.data), current_block_size);
        if (!file.good() && !file.eof()) {
            cerr << "Error reading block " << i << endl;
            return manager;
        }

        manager.blocks.push_back(move(block));
    }

    cout << "Divided file into " << manager.blocks.size() << " blocks of size "
        << block_size << " bytes" << endl;

    return manager;
}

// Reassembles blocks back into original file
bool reassemble_blocks(const BlockManager& manager, const string& output_filename) {
    ofstream file(output_filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening output file: " << output_filename << endl;
        return false;
    }

    // Write each block to the output file
    for (const auto& block : manager.blocks) {
        file.write(reinterpret_cast<char*>(block.data), block.size);
        if (!file.good()) {
            cerr << "Error writing block to output file" << endl;
            return false;
        }
    }

    return true;
}