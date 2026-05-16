#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
using namespace std;

// Canonical Huffman coding with self-contained serialization.
// Format: [256 code-length bytes][1 pad byte][packed bit data]
vector<unsigned char> huffman_compress(const vector<unsigned char>& input);
vector<unsigned char> huffman_decompress(const vector<unsigned char>& input);

#endif
