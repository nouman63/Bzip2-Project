CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2
TARGET   = bzip2_impl
SRCS     = main.cpp compressor.cpp rle1.cpp bwt.cpp huffman.cpp block_management.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test: $(TARGET)
	./$(TARGET) -t test.txt test.bz2p
	@echo "Test complete."

clean:
	rm -f $(OBJS) $(TARGET) *.bz2p *.recovered

.PHONY: all test clean
