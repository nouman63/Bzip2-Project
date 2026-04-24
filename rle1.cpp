#include "rle1.h"
#include <iostream>

using namespace std;

vector<unsigned char> rle1_encode(const vector<unsigned char>& input) {
    vector<unsigned char> output;

    if (input.empty()) return output;

    size_t i = 0;
    while (i < input.size()) {
        unsigned char current = input[i];
        size_t count = 1;

        // Count consecutive identical bytes (max 255)
        while (i + count < input.size() && input[i + count] == current && count < 255) {
            count++;
        }

        // Add count and character to output
        output.push_back(static_cast<unsigned char>(count));
        output.push_back(current);

        i += count;
    }

    return output;
}

vector<unsigned char> rle1_decode(const vector<unsigned char>& input) {
    vector<unsigned char> output;

    if (input.empty()) return output;

    for (size_t i = 0; i + 1 < input.size(); i += 2) {
        unsigned char count = input[i];
        unsigned char value = input[i + 1];

        for (unsigned char j = 0; j < count; j++) {
            output.push_back(value);
        }
    }

    return output;
}