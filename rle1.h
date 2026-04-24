#ifndef RLE1_H
#define RLE1_H

#include <cstddef>
#include <vector>

using namespace std;

// Encodes data using Run-Length Encoding
// Input: raw data bytes
// Returns: encoded data
vector<unsigned char> rle1_encode(const vector<unsigned char>& input);

// Decodes RLE-1 encoded data
// Input: encoded data
// Returns: decoded original data
vector<unsigned char> rle1_decode(const vector<unsigned char>& input);

#endif // RLE1_H#pragma once
