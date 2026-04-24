#include "bwt.h"
#include <algorithm>

using namespace std;

// Compare two rotations for sorting
static bool compare_rotations(const Rotation& a, const Rotation& b) {
    return a.rotation < b.rotation;
}

// Matrix-based BWT encoding
BWTResult bwt_encode(const vector<unsigned char>& input) {
    BWTResult result;

    if (input.empty()) return result;

    size_t len = input.size();
    string s(input.begin(), input.end());

    // Create all rotations
    vector<Rotation> rotations;
    rotations.reserve(len);

    for (size_t i = 0; i < len; i++) {
        string rotation = s.substr(i) + s.substr(0, i);
        rotations.emplace_back(rotation, i);
    }

    // Sort rotations lexicographically
    sort(rotations.begin(), rotations.end(), compare_rotations);

    // Extract last column and find primary index
    result.data.resize(len);
    for (size_t i = 0; i < len; i++) {
        result.data[i] = rotations[i].rotation.back();
        if (rotations[i].index == 0) {
            result.primary_index = i;
        }
    }

    return result;
}

// Inverse BWT transform
vector<unsigned char> bwt_decode(const vector<unsigned char>& input, int primary_index) {
    vector<unsigned char> output;

    if (input.empty()) return output;

    size_t len = input.size();

    // Create table of characters
    vector<pair<unsigned char, int>> table(len);
    for (size_t i = 0; i < len; i++) {
        table[i] = { input[i], i };
    }

    // Sort by character
    sort(table.begin(), table.end());

    // Reconstruct original string
    output.reserve(len);
    int next = primary_index;
    for (size_t i = 0; i < len; i++) {
        next = table[next].second;
        output.push_back(table[next].first);
    }

    return output;
}