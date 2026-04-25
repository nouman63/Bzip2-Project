#include "bzip2.h"
#include <algorithm>
#include <vector>

// ============= BLOCK MANAGEMENT =============

BlockManager* divide_into_blocks(const char* filename, size_t block_size) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0 || !file) return NULL;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int num_blocks = (int)((file_size + block_size - 1) / block_size);

    BlockManager* manager = (BlockManager*)malloc(sizeof(BlockManager));
    if (!manager) {
        fclose(file);
        return NULL;
    }

    manager->blocks = (Block*)malloc(num_blocks * sizeof(Block));
    if (!manager->blocks) {
        free(manager);
        fclose(file);
        return NULL;
    }

    manager->num_blocks = num_blocks;
    manager->block_size = block_size;

    for (int i = 0; i < num_blocks; i++) {
        size_t current_size = (i == num_blocks - 1) ?
            (size_t)(file_size - i * block_size) : block_size;

        manager->blocks[i].data = (unsigned char*)malloc(current_size);
        if (!manager->blocks[i].data) {
            for (int j = 0; j < i; j++) free(manager->blocks[j].data);
            free(manager->blocks);
            free(manager);
            fclose(file);
            return NULL;
        }

        manager->blocks[i].size = current_size;
        manager->blocks[i].original_size = current_size;

        fread(manager->blocks[i].data, 1, current_size, file);
    }

    fclose(file);
    return manager;
}

int reassemble_blocks(BlockManager* manager, const char* output_filename) {
    FILE* file;
    errno_t err = fopen_s(&file, output_filename, "wb");
    if (err != 0 || !file) return -1;

    for (int i = 0; i < manager->num_blocks; i++) {
        fwrite(manager->blocks[i].data, 1, manager->blocks[i].size, file);
    }

    fclose(file);
    return 0;
}

void free_block_manager(BlockManager* manager) {
    if (!manager) return;
    for (int i = 0; i < manager->num_blocks; i++) {
        if (manager->blocks[i].data) free(manager->blocks[i].data);
    }
    if (manager->blocks) free(manager->blocks);
    free(manager);
}

// ============= RLE-1 ENCODING/DECODING =============

vector<unsigned char> rle1_encode(const vector<unsigned char>& input) {
    vector<unsigned char> output;
    size_t i = 0;

    while (i < input.size()) {
        unsigned char current = input[i];
        int count = 1;

        while (i + count < input.size() && input[i + count] == current && count < 255) {
            count++;
        }

        output.push_back((unsigned char)count);
        output.push_back(current);

        i += count;
    }

    return output;
}

vector<unsigned char> rle1_decode(const vector<unsigned char>& input) {
    vector<unsigned char> output;
    size_t i = 0;

    while (i < input.size()) {
        unsigned char count = input[i++];
        unsigned char character = input[i++];

        for (int j = 0; j < count; j++) {
            output.push_back(character);
        }
    }

    return output;
}

// ============= BWT ENCODING/DECODING (CORRECTED) =============

// Structure for BWT rotations using vector for proper byte comparison
struct RotationData {
    vector<unsigned char> rotation;
    int index;

    bool operator<(const RotationData& other) const {
        return rotation < other.rotation;
    }
};

vector<unsigned char> bwt_encode(const vector<unsigned char>& input, int& primary_index) {
    vector<unsigned char> output;
    size_t len = input.size();

    if (len == 0) return output;

    // Create all rotations
    vector<RotationData> rotations(len);
    for (size_t i = 0; i < len; i++) {
        rotations[i].rotation.resize(len);
        for (size_t j = 0; j < len; j++) {
            rotations[i].rotation[j] = input[(i + j) % len];
        }
        rotations[i].index = (int)i;
    }

    // Sort rotations
    sort(rotations.begin(), rotations.end());

    // Find primary index (where original string appears)
    for (size_t i = 0; i < len; i++) {
        if (rotations[i].index == 0) {
            primary_index = (int)i;
            break;
        }
    }

    // Extract last column
    output.resize(len);
    for (size_t i = 0; i < len; i++) {
        output[i] = rotations[i].rotation[len - 1];
    }

    return output;
}

vector<unsigned char> bwt_decode(const vector<unsigned char>& input, int primary_index) {
    vector<unsigned char> output;
    size_t len = input.size();

    if (len == 0) return output;

    // Initialize table with empty strings
    vector<vector<unsigned char>> table(len, vector<unsigned char>());

    // Reconstruct the table
    for (size_t i = 0; i < len; i++) {
        // Prepend the BWT column to each row
        for (size_t j = 0; j < len; j++) {
            table[j].insert(table[j].begin(), input[j]);
        }

        // Sort rows
        sort(table.begin(), table.end());
    }

    // Get the original string from the primary_index row
    output = table[primary_index];

    return output;
}