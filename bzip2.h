#ifndef BZIP2_H
#define BZIP2_H

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

// Block structures
typedef struct {
    unsigned char* data;
    size_t size;
    size_t original_size;
} Block;

typedef struct {
    Block* blocks;
    int num_blocks;
    size_t block_size;
} BlockManager;

// BWT structure
typedef struct {
    char* rotation;
    int index;
} Rotation;

// Function prototypes - C++ vector versions
vector<unsigned char> rle1_encode(const vector<unsigned char>& input);
vector<unsigned char> rle1_decode(const vector<unsigned char>& input);

vector<unsigned char> bwt_encode(const vector<unsigned char>& input, int& primary_index);
vector<unsigned char> bwt_decode(const vector<unsigned char>& input, int primary_index);

// Block management - C style (kept as is)
BlockManager* divide_into_blocks(const char* filename, size_t block_size);
int reassemble_blocks(BlockManager* manager, const char* output_filename);
void free_block_manager(BlockManager* manager);

#endif#pragma once
