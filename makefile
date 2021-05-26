TARGET_EXEC_SERVER = screen-worms-server

BUILD_DIR_SERVER = ./build_server
SRC_DIRS_SERVER = ./src/server ./src/shared_functionalities

SRCS_SERVER := $(shell find $(SRC_DIRS_SERVER) -name *.cpp -or -name *.c)
OBJS_SERVER := $(SRCS:%=$(BUILD_DIR_SERVER)/%.o)
DEPS_SERVER := $(OBJS_SERVER:.o=.d)

INC_DIRS_SERVER := $(shell find $(SRC_DIRS_SERVER) -type d)
INC_FLAGS_SERVER := $(addprefix -I,$(INC_DIRS_SERVER))

CC = gcc
CXX = g++

CPPFLAGS_SERVER = $(INC_FLAGS_SERVER) -MMD -MP
CFLAGS = -Wall -Wextra -std=c11
CXX_FLAGS = -Wall -Wextra -std=c++17

LIBS =

$(TARGET_EXEC_SERVER): $(OBJS_SERVER)
	$(CXX) $(OBJS_SERVER) -o $@ $(LDFLAGS) $(LIBS)

# c source
$(BUILD_DIR_SERVER)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS_SERVER) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR_SERVER)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS_SERVER) $(CXX_FLAGS) -c $< -o $@ $(LIBS)


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR_SERVER)
	rm screen-worms-server

-include $(DEPS_SERVER)

MKDIR_P = mkdir -p