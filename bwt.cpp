#include "bwt.h"
#include <algorithm>
#include <numeric>

using namespace std;

// BWT encode using prefix doubling (O(n log^2 n)).
// Each round doubles the comparison window and re-ranks positions,
// replacing the O(n^2 log n) character-by-character sort.
BWTResult bwt_encode(const vector<unsigned char>& input) {
    BWTResult result;
    if (input.empty()) return result;

    int n = (int)input.size();
    vector<int> sa(n), rank_(n), tmp(n);

    iota(sa.begin(), sa.end(), 0);
    for (int i = 0; i < n; i++) rank_[i] = input[i];

    for (int gap = 1; gap < n; gap <<= 1) {
        // Sort by (rank[i], rank[(i+gap) % n]) — cyclic window of width 2*gap
        auto cmp = [&](int a, int b) {
            if (rank_[a] != rank_[b]) return rank_[a] < rank_[b];
            return rank_[(a + gap) % n] < rank_[(b + gap) % n];
        };
        sort(sa.begin(), sa.end(), cmp);

        // Reassign ranks: equal pairs get the same rank
        tmp[sa[0]] = 0;
        for (int i = 1; i < n; i++)
            tmp[sa[i]] = tmp[sa[i-1]] + (cmp(sa[i-1], sa[i]) ? 1 : 0);
        rank_ = tmp;

        if (rank_[sa[n-1]] == n-1) break;  // all ranks unique — done early
    }

    result.data.resize(n);
    for (int i = 0; i < n; i++) {
        if (sa[i] == 0) result.primary_index = i;
        result.data[i] = input[(sa[i] + n - 1) % n];
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
