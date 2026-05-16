#include "huffman.h"
#include <algorithm>
#include <queue>
#include <cstring>

using namespace std;

// Internal tree node (index-based, avoids unique_ptr UB on priority_queue::top)
struct HNode {
    int sym;   // -1 for internal nodes
    int freq;
    int left, right;  // indices into node pool, -1 if absent
};

static int build_tree(int freq[256], vector<HNode>& nodes) {
    nodes.clear();

    // (freq, node_index) min-heap
    auto cmp = [](pair<int,int> a, pair<int,int> b){ return a.first > b.first; };
    priority_queue<pair<int,int>, vector<pair<int,int>>, decltype(cmp)> pq(cmp);

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            nodes.push_back({i, freq[i], -1, -1});
            pq.push({freq[i], (int)nodes.size()-1});
        }
    }

    if (nodes.empty()) return -1;

    // Single-symbol: assign it length 1
    if (pq.size() == 1) {
        auto [f, idx] = pq.top(); pq.pop();
        nodes.push_back({-1, f, idx, -1});
        return (int)nodes.size()-1;
    }

    while (pq.size() > 1) {
        auto [fa, a] = pq.top(); pq.pop();
        auto [fb, b] = pq.top(); pq.pop();
        nodes.push_back({-1, fa+fb, a, b});
        pq.push({fa+fb, (int)nodes.size()-1});
    }
    return pq.top().second;
}

static void assign_lengths(const vector<HNode>& nodes, int idx, int depth, unsigned char lengths[256]) {
    if (idx < 0) return;
    const HNode& n = nodes[idx];
    if (n.sym >= 0) {
        lengths[n.sym] = (unsigned char)min(depth, 255);
        return;
    }
    assign_lengths(nodes, n.left,  depth+1, lengths);
    assign_lengths(nodes, n.right, depth+1, lengths);
}

// Generate canonical codes from code-length array.
// Canonical property: sort by (length, symbol), assign codes starting from 0,
// left-shifting when length increases.
static void make_canonical(unsigned char lengths[256], unsigned int codes[256]) {
    int order[256], cnt = 0;
    for (int i = 0; i < 256; i++) if (lengths[i] > 0) order[cnt++] = i;
    sort(order, order+cnt, [&](int a, int b){
        return lengths[a] < lengths[b] || (lengths[a] == lengths[b] && a < b);
    });

    memset(codes, 0, 256*sizeof(unsigned int));
    unsigned int code = 0;
    unsigned char prev_len = 0;
    for (int i = 0; i < cnt; i++) {
        int sym = order[i];
        unsigned char len = lengths[sym];
        if (prev_len > 0) code = (code+1) << (len - prev_len);
        codes[sym] = code;
        prev_len = len;
    }
}

// Output format: 256 code-length bytes | 1 pad byte | packed bit data
vector<unsigned char> huffman_compress(const vector<unsigned char>& input) {
    if (input.empty()) return {};

    int freq[256] = {};
    for (unsigned char c : input) freq[c]++;

    vector<HNode> nodes;
    int root = build_tree(freq, nodes);

    unsigned char lengths[256] = {};
    assign_lengths(nodes, root, 0, lengths);

    unsigned int codes[256] = {};
    make_canonical(lengths, codes);

    vector<unsigned char> output;
    output.reserve(257 + input.size() / 2);

    // Header: 256 code-length bytes
    for (int i = 0; i < 256; i++) output.push_back(lengths[i]);

    // Placeholder for pad byte (filled in after encoding)
    output.push_back(0);
    size_t pad_pos = output.size() - 1;

    // Bit-pack the encoded data using 64-bit buffer to avoid overflow
    unsigned long long buf = 0;
    int bits = 0;
    for (unsigned char c : input) {
        int len = lengths[c];
        buf = (buf << len) | (unsigned long long)codes[c];
        bits += len;
        while (bits >= 8) {
            bits -= 8;
            output.push_back((unsigned char)(buf >> bits));
            buf &= (1ULL << bits) - 1;
        }
    }

    // Flush remaining bits with zero padding
    unsigned char pad = 0;
    if (bits > 0) {
        pad = (unsigned char)(8 - bits);
        output.push_back((unsigned char)(buf << pad));
    }
    output[pad_pos] = pad;

    return output;
}

vector<unsigned char> huffman_decompress(const vector<unsigned char>& input) {
    if (input.size() < 257) return {};  // need 256 lengths + 1 pad byte

    unsigned char lengths[256];
    for (int i = 0; i < 256; i++) lengths[i] = input[i];
    unsigned char pad = input[256];

    int max_len = 0;
    for (int i = 0; i < 256; i++) if (lengths[i] > max_len) max_len = lengths[i];
    if (max_len == 0) return {};

    unsigned int codes[256] = {};
    make_canonical(lengths, codes);

    vector<unsigned char> output;

    if (max_len <= 16) {
        // Fast O(1) lookup table: indexed by top max_len bits of bit stream
        int table_size = 1 << max_len;
        vector<int> tbl_sym(table_size, -1);
        vector<int> tbl_len(table_size, 0);

        for (int sym = 0; sym < 256; sym++) {
            if (lengths[sym] == 0) continue;
            unsigned int base = codes[sym] << (max_len - lengths[sym]);
            unsigned int span = 1u << (max_len - lengths[sym]);
            for (unsigned int x = 0; x < span; x++) {
                tbl_sym[base+x] = sym;
                tbl_len[base+x] = lengths[sym];
            }
        }

        unsigned long long buf = 0;
        int bits = 0;
        size_t i = 257;
        size_t data_end = input.size();

        while (i < data_end || bits > 0) {
            while (bits < max_len + 8 && i < data_end) {
                buf = (buf << 8) | (unsigned long long)input[i++];
                bits += 8;
            }

            int usable = (i >= data_end) ? (bits - (int)pad) : bits;
            if (usable <= 0) break;

            int peek = (usable >= max_len) ? max_len : usable;
            unsigned int idx = (unsigned int)(buf >> (bits - peek)) << (max_len - peek);
            idx &= (unsigned int)(table_size - 1);

            int sym  = tbl_sym[idx];
            int blen = tbl_len[idx];

            if (sym < 0 || blen > usable) break;

            output.push_back((unsigned char)sym);
            bits -= blen;
            buf = (bits < 64) ? (buf & ((1ULL << bits) - 1)) : 0;
        }
    } else {
        // Fallback: linear scan for very deep trees (rare)
        // Collect active symbols sorted by (length, symbol)
        int order[256], cnt = 0;
        for (int i = 0; i < 256; i++) if (lengths[i] > 0) order[cnt++] = i;
        sort(order, order+cnt, [&](int a, int b){
            return lengths[a] < lengths[b] || (lengths[a] == lengths[b] && a < b);
        });

        unsigned long long buf = 0;
        int bits = 0;
        size_t i = 257;
        size_t data_end = input.size();

        while (i < data_end || bits > 0) {
            while (bits < 24 && i < data_end) {
                buf = (buf << 8) | (unsigned long long)input[i++];
                bits += 8;
            }
            int usable = (i >= data_end) ? (bits - (int)pad) : bits;
            if (usable <= 0) break;

            bool matched = false;
            for (int k = 0; k < cnt && !matched; k++) {
                int sym = order[k];
                int blen = lengths[sym];
                if (blen > usable) break;
                unsigned int extracted = (unsigned int)(buf >> (bits - blen)) & ((1u << blen) - 1);
                if (extracted == codes[sym]) {
                    output.push_back((unsigned char)sym);
                    bits -= blen;
                    buf = (bits < 64) ? (buf & ((1ULL << bits) - 1)) : 0;
                    matched = true;
                }
            }
            if (!matched) break;
        }
    }

    return output;
}
