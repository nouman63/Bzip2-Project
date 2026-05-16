#include "bwt.h"
#include <algorithm>
#include <numeric>

using namespace std;

// Sort indices using virtual cyclic-rotation comparison on the original buffer.
// O(n) space (indices only), avoids the O(n^2) memory of storing all rotations.
// Uses unsigned char comparison so bytes 0x80-0xFF sort correctly (no signed-char bug).
BWTResult bwt_encode(const vector<unsigned char>& input) {
    BWTResult result;
    if (input.empty()) return result;

    size_t len = input.size();
    vector<int> indices(len);
    iota(indices.begin(), indices.end(), 0);

    sort(indices.begin(), indices.end(), [&](int a, int b) {
        for (size_t k = 0; k < len; k++) {
            unsigned char ca = input[(a + k) % len];
            unsigned char cb = input[(b + k) % len];
            if (ca != cb) return ca < cb;
        }
        return a < b;
    });

    result.data.resize(len);
    for (size_t i = 0; i < len; i++) {
        if (indices[i] == 0) result.primary_index = (int)i;
        result.data[i] = input[(indices[i] + len - 1) % len];
    }
    return result;
}

// Inverse BWT using the T-mapping (LF-mapping) algorithm.
// T[i] = the position in sorted-F that L[i] maps to (stable sort property).
// Reconstruct going right-to-left: output[i] = L[idx]; idx = T[idx].
vector<unsigned char> bwt_decode(const vector<unsigned char>& input, int primary_index) {
    if (input.empty()) return {};

    size_t len = input.size();

    int freq[256] = {};
    for (unsigned char c : input) freq[c]++;

    // Start positions of each byte in sorted order (first column F)
    int start[256];
    start[0] = 0;
    for (int c = 1; c < 256; c++) start[c] = start[c-1] + freq[c-1];

    // Build T-mapping: T[i] = F-position that L[i] maps to
    vector<int> T(len);
    int cur[256];
    copy(start, start + 256, cur);
    for (size_t i = 0; i < len; i++)
        T[i] = cur[(unsigned char)input[i]]++;

    // Reconstruct original string right-to-left
    vector<unsigned char> output(len);
    int idx = primary_index;
    for (int i = (int)len - 1; i >= 0; i--) {
        output[i] = input[idx];
        idx = T[idx];
    }
    return output;
}
