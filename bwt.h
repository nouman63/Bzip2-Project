#ifndef BWT_H
#define BWT_H

#include <cstddef>
#include <vector>
#include <string>

using namespace std;

// Rotation structure for BWT
struct Rotation {
    string rotation;
    int index;

    Rotation(const string& rot, int idx) : rotation(rot), index(idx) {}
};

// Forward BWT transform
// Returns: BWT encoded data and primary index
struct BWTResult {
    vector<unsigned char> data;
    int primary_index;
};

BWTResult bwt_encode(const vector<unsigned char>& input);

// Inverse BWT transform
vector<unsigned char> bwt_decode(const vector<unsigned char>& input, int primary_index);

#endif // BWT_H#pragma once
