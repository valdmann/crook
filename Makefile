CXXFLAGS := -O3 -s -fno-exceptions -finline-limit=10000 -fwhole-program -Wall -Wextra
# CXXFLAGS := -g -Wall -Wextra

.PHONY: all
all : crook

.PHONY: test
test : crook
	@./crook c data data.enc
	@./crook d data.enc data.dec
	@cmp data data.dec

.PHONY: clean
clean:
	rm -f crook data.enc data.dec

.PHONY: check-syntax
check-syntax:
	$(CXX) -Wall -Wextra -fsyntax-only $(CHK_SOURCES)

crook : crook.cpp *.hpp Makefile
	$(CXX) $(CXXFLAGS) $< -o $@
