CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS  :=

SRCS := src/lexer.cpp src/parser.cpp src/sema.cpp src/codegen.cpp src/main.cpp
OBJS := $(SRCS:.cpp=.o)
HDRS := src/token.hpp src/lexer.hpp src/ast.hpp src/parser.hpp src/sema.hpp src/codegen.hpp

BIN := amethystc

.PHONY: all clean examples test deb

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

src/%.o: src/%.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

examples: $(BIN)
	@mkdir -p build
	@for f in examples/*.amt; do \
		echo "  AMETHYST $$f"; \
		./$(BIN) -o build/$$(basename $$f .amt) $$f || exit 1; \
	done

test: examples
	@echo "--- running tests ---"
	@./scripts/run_tests.sh

deb:
	@./scripts/package_deb.sh

clean:
	rm -f $(OBJS) $(BIN)
	rm -rf build dist
	rm -f examples/*.s examples/*.o
