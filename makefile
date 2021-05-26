TARGET_EXEC = screen-worms-server

BUILD_DIR = ./build
SRC_DIRS = ./src/server ./src/shared_functionalities

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC = gcc
CXX = g++

CPPFLAGS = $(INC_FLAGS) -MMD -MP
CFLAGS = -Wall -Wextra -std=c11
CXX_FLAGS = -Wall -Wextra -std=c++17

LIBS = -lstdc++fs

$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXX_FLAGS) -c $< -o $@ $(LIBS)


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)
	rm screen-worms-server

-include $(DEPS)

MKDIR_P = mkdir -p