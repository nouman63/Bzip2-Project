#include "huffman.h"
#include <queue>
#include <map>

using namespace std;

// Comparison for priority queue
struct CompareNode {
    bool operator()(const unique_ptr<HuffmanNode>& a, const unique_ptr<HuffmanNode>& b) {
        return a->frequency > b->frequency;
    }
};

// Generate Huffman codes recursively
static void generate_codes(const HuffmanNode* node, vector<bool> code,
    unordered_map<unsigned char, vector<bool>>& codes) {
    if (!node) return;

    if (node->isLeaf()) {
        codes[node->character] = code;
        return;
    }

    if (node->left) {
        vector<bool> left_code = code;
        left_code.push_back(false);
        generate_codes(node->left.get(), left_code, codes);
    }

    if (node->right) {
        vector<bool> right_code = code;
        right_code.push_back(true);
        generate_codes(node->right.get(), right_code, codes);
    }
}

// Huffman encoding
HuffmanResult huffman_encode(const vector<unsigned char>& input) {
    HuffmanResult result;

    if (input.empty()) return result;

    // Count frequencies
    unordered_map<unsigned char, int> frequencies;
    for (unsigned char c : input) {
        frequencies[c]++;
    }

    // Create priority queue
    priority_queue<unique_ptr<HuffmanNode>,
        vector<unique_ptr<HuffmanNode>>,
        CompareNode> pq;

    for (const auto& pair : frequencies) {
        pq.push(make_unique<HuffmanNode>(pair.first, pair.second));
    }

    // Build Huffman tree
    while (pq.size() > 1) {
        auto left = move(const_cast<unique_ptr<HuffmanNode>&>(pq.top()));
        pq.pop();
        auto right = move(const_cast<unique_ptr<HuffmanNode>&>(pq.top()));
        pq.pop();

        int combined_freq = left->frequency + right->frequency;
        auto parent = make_unique<HuffmanNode>(combined_freq, move(left), move(right));
        pq.push(move(parent));
    }

    if (!pq.empty()) {
        result.root = move(const_cast<unique_ptr<HuffmanNode>&>(pq.top()));
    }

    // Generate codes
    generate_codes(result.root.get(), vector<bool>(), result.codes);

    // Encode data
    for (unsigned char c : input) {
        const auto& code = result.codes[c];
        result.encoded_data.insert(result.encoded_data.end(), code.begin(), code.end());
    }

    return result;
}

// Huffman decoding
vector<unsigned char> huffman_decode(const vector<bool>& encoded_data, HuffmanNode* root) {
    vector<unsigned char> output;

    if (encoded_data.empty() || !root) return output;

    const HuffmanNode* current = root;
    for (bool bit : encoded_data) {
        if (bit) {
            current = current->right.get();
        }
        else {
            current = current->left.get();
        }

        if (current->isLeaf()) {
            output.push_back(current->character);
            current = root;
        }
    }

    return output;
}