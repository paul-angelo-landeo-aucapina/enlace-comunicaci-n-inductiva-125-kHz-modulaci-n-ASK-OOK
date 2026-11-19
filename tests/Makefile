CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++17

all: test_rx_decoder test_tx_encoder

test_rx_decoder: test_rx_decoder.cpp ../firmware_rx/manchester_rx.cpp ../firmware_rx/packet_parser.cpp
	$(CXX) $(CXXFLAGS) -I ../firmware_rx $^ -o $@

test_tx_encoder: test_tx_encoder.cpp ../firmware_tx/packet_builder.cpp ../firmware_tx/manchester_tx.cpp ../firmware_rx/manchester_rx.cpp ../firmware_rx/packet_parser.cpp
	$(CXX) $(CXXFLAGS) -I ../firmware_tx -I ../firmware_rx $^ -o $@

test: all
	@echo "=== Running RX Tests ==="
	./test_rx_decoder
	@echo ""
	@echo "=== Running TX & Loopback Tests ==="
	./test_tx_encoder

clean:
	rm -f test_rx_decoder test_tx_encoder

.PHONY: all test clean
