CXX ?= g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -Iinclude -Ithird_party/raylib/src
LDFLAGS = third_party/raylib/src/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

SRCS = src/main.cpp src/pieces.cpp src/pit.cpp src/game.cpp src/renderer.cpp src/audio.cpp src/config.cpp
OBJS = $(SRCS:src/%.cpp=obj/%.o)
TARGET = bin/blockout

RAYLIB_VERSION ?= 5.5
RAYLIB_DIR = third_party/raylib
RAYLIB_LIB = $(RAYLIB_DIR)/src/libraylib.a

.PHONY: all clean run deps raylib

all: deps $(TARGET)

deps: raylib

raylib:
	@if [ ! -f $(RAYLIB_DIR)/src/raylib.h ]; then \
		echo "Raylib source not found in $(RAYLIB_DIR). Fetching raylib $(RAYLIB_VERSION)..."; \
		TEMP_CLONE=$$(mktemp -d); \
		git clone --depth 1 --branch $(RAYLIB_VERSION) https://github.com/raysan5/raylib.git $$TEMP_CLONE && \
		rm -rf $$TEMP_CLONE/.git && \
		cp -rf $$TEMP_CLONE/* $(RAYLIB_DIR)/ && \
		rm -rf $$TEMP_CLONE; \
	fi
	@if [ ! -f $(RAYLIB_LIB) ]; then \
		echo "Building raylib static library..."; \
		$(MAKE) -C $(RAYLIB_DIR)/src PLATFORM=PLATFORM_DESKTOP -j$$(nproc); \
	fi

$(TARGET): $(OBJS) | bin
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	@echo "Build complete: $(TARGET)"

obj/%.o: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin:
	mkdir -p bin

obj:
	mkdir -p obj

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
