#include "bzip2.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

void print_vector(const vector<unsigned char>& data, const string& label) {
    cout << label << " (size: " << data.size() << "): ";
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            cout << data[i];
        }
        else {
            printf("[%02X]", data[i]);
        }
    }
    cout << endl;
}

// Function to save compressed data to a text file
void save_to_file(const vector<unsigned char>& data, const string& filename, const string& description) {
    FILE* file;
    errno_t err = fopen_s(&file, filename.c_str(), "w");
    if (err != 0 || !file) {
        cout << "Error: Cannot create file " << filename << endl;
        return;
    }

    fprintf(file, "=== %s ===\n", description.c_str());
    fprintf(file, "Size: %zu bytes\n", data.size());
    fprintf(file, "Data: ");

    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            fprintf(file, "%c", data[i]);
        }
        else {
            fprintf(file, "[%02X]", data[i]);
        }
    }

    fclose(file);
    cout << "Saved to: " << filename << endl;
}

// Function to calculate and display compression ratio
void calculate_compression_ratio(size_t original_size, size_t compressed_size) {
    if (original_size == 0) return;

    double ratio = (double)compressed_size / original_size;
    double percentage = (1.0 - ratio) * 100;

    cout << fixed << setprecision(2);
    cout << "\nCompression Statistics:" << endl;
    cout << "Original size: " << original_size << " bytes" << endl;
    cout << "Compressed size: " << compressed_size << " bytes" << endl;
    cout << "Compression ratio: " << ratio << ":1" << endl;
    cout << "Space saved: " << percentage << "%" << endl;
}

int main() {
    printf("========================================\n");
    printf("BZIP2 COMPRESSION DEMO\n");
    printf("========================================\n\n");

    //// Test RLE-1
    //printf("=== RLE-1 TEST ===\n");
    //string test_str = "ABBBCCCCD";
    //vector<unsigned char> original(test_str.begin(), test_str.end());

    //vector<unsigned char> encoded = rle1_encode(original);
    //vector<unsigned char> decoded = rle1_decode(encoded);

    //print_vector(original, "Original");
    //print_vector(encoded, "Encoded");
    //print_vector(decoded, "Decoded");

    //// Save RLE results to file
    //save_to_file(encoded, "rle1_output.txt", "RLE-1 Compressed Data");

    //calculate_compression_ratio(original.size(), encoded.size());

    //if (original == decoded) {
    //    printf("✓ RLE-1 Test PASSED\n\n");
    //}
    //else {
    //    printf("✗ RLE-1 Test FAILED\n\n");
    //}

    //// Test BWT
    //printf("=== BWT TEST ===\n");
    //string bwt_str = "banana";
    //vector<unsigned char> bwt_original(bwt_str.begin(), bwt_str.end());
    //int primary_index = 0;

    //vector<unsigned char> bwt_encoded = bwt_encode(bwt_original, primary_index);
    //vector<unsigned char> bwt_decoded = bwt_decode(bwt_encoded, primary_index);

    //print_vector(bwt_original, "Original");
    //cout << "Primary index: " << primary_index << endl;
    //print_vector(bwt_encoded, "BWT Encoded");
    //print_vector(bwt_decoded, "BWT Decoded");

    //// Save BWT results to file
    //save_to_file(bwt_encoded, "bwt_output.txt", "BWT Encoded Data");

    //if (bwt_original == bwt_decoded) {
    //    printf("✓ BWT Test PASSED\n\n");
    //}
    //else {
    //    printf("✗ BWT Test FAILED\n\n");
    //}

    // Test complete pipeline with block division
    printf("=== COMPLETE PIPELINE TEST ===\n");

    //// Create test file
    //FILE* test;
    //errno_t err = fopen_s(&test, "test.txt", "w");
    //if (err == 0 && test != NULL) {
    //    fprintf(test, "ABBBCCCCD");
    //    fclose(test);
    //    printf("Test file 'test.txt' created successfully\n");
    //}

    BlockManager* manager = divide_into_blocks("test.txt", 100 * 1024);
    if (manager) {
        printf("File divided into %d block(s)\n", manager->num_blocks);

        for (int i = 0; i < manager->num_blocks; i++) {
            printf("\n--- Processing Block %d ---\n", i);

            // Convert block to vector
            vector<unsigned char> block_data(
                manager->blocks[i].data,
                manager->blocks[i].data + manager->blocks[i].size
            );

            print_vector(block_data, "Original");

            // Compress: RLE -> BWT
            vector<unsigned char> rle_result = rle1_encode(block_data);
            int prim_idx;
            vector<unsigned char> bwt_result = bwt_encode(rle_result, prim_idx);

            printf("After compression: %zu bytes\n", bwt_result.size());

            // Save the final compressed data to file
            save_to_file(bwt_result, "compressed_output.txt", "Final Compressed Data (RLE + BWT)");

            // Decompress: BWT -> RLE
            vector<unsigned char> bwt_decompress = bwt_decode(bwt_result, prim_idx);
            vector<unsigned char> rle_decompress = rle1_decode(bwt_decompress);

            print_vector(rle_decompress, "Decompressed");

            // Verify
            if (block_data == rle_decompress) {
                printf("✓ Block %d: Successfully compressed and decompressed!\n", i);
            }
            else {
                printf("✗ Block %d: Compression/Decompression failed!\n", i);
            }
        }

        free_block_manager(manager);
    }

    printf("\n========================================\n");
    printf("Output files created:\n");
    printf("  - rle1_output.txt (RLE compressed data)\n");
    printf("  - bwt_output.txt (BWT encoded data)\n");
    printf("  - compressed_output.txt (Final compressed data)\n");
    printf("========================================\n");

    return 0;
}