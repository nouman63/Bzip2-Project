#include "rle1.h"

// BZip2 RLE-1: only encodes runs of 4+ identical bytes.
// Run of >=4: emit 4 literal bytes + 1 count byte (= extras beyond 4, range 0-255 -> total 4-259)
// Everything else: copy verbatim. This NEVER expands the data.
vector<unsigned char> rle1_encode(const vector<unsigned char>& input) {
    vector<unsigned char> output;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c = input[i];
        size_t run = 1;
        while (i + run < input.size() && input[i + run] == c && run < 259)
            run++;
        if (run >= 4) {
            output.push_back(c);
            output.push_back(c);
            output.push_back(c);
            output.push_back(c);
            output.push_back((unsigned char)(run - 4));
            i += run;
        } else {
            output.push_back(input[i++]);
        }
    }
    return output;
}

// Decode: detect 4-consecutive-same-bytes in the growing output, then read next input byte as count.
vector<unsigned char> rle1_decode(const vector<unsigned char>& input) {
    vector<unsigned char> output;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c = input[i++];
        output.push_back(c);
        size_t sz = output.size();
        if (sz >= 4 &&
            output[sz-1] == output[sz-2] &&
            output[sz-2] == output[sz-3] &&
            output[sz-3] == output[sz-4]) {
            if (i < input.size()) {
                unsigned char extra = input[i++];
                for (unsigned char k = 0; k < extra; k++)
                    output.push_back(c);
            }
        }
    }
    return output;
}
