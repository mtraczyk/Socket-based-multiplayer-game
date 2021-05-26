TARGET_EXEC_SERVER = screen-worms-server
TARGET_EXEC_CLIENT = screen-worms-client

BUILD_DIR_SERVER = ./build_server
SRC_DIRS_SERVER = ./src/server ./src/shared_functionalities
BUILD_DIR_CLIENT = ./build_client
SRC_DIRS_CLIENT = ./src/client ./src/shared_functionalities

SRCS_SERVER := $(shell find $(SRC_DIRS_SERVER) -name *.cpp -or -name *.c)
OBJS_SERVER := $(SRCS_SERVER:%=$(BUILD_DIR_SERVER)/%.o)
DEPS_SERVER := $(OBJS_SERVER:.o=.d)
SRCS_CLIENT := $(shell find $(SRC_DIRS_CLIENT) -name *.cpp -or -name *.c)
OBJS_CLIENT := $(SRCS_SERVER:%=$(BUILD_DIR_CLIENT)/%.o)
DEPS_CLIENT := $(OBJS_CLIENT:.o=.d)

INC_DIRS_SERVER := $(shell find $(SRC_DIRS_SERVER) -type d)
INC_FLAGS_SERVER := $(addprefix -I,$(INC_DIRS_SERVER))
INC_DIRS_CLIENT := $(shell find $(SRC_DIRS_CLIENT) -type d)
INC_FLAGS_CLIENT := $(addprefix -I,$(INC_DIRS_CLIENT))

CC = gcc
CXX = g++

CPPFLAGS_SERVER = $(INC_FLAGS_SERVER) -MMD -MP
CPPFLAGS_CLIENT = $(INC_FLAGS_CLIENT) -MMD -MP
CFLAGS = -Wall -Wextra -std=c11
CXX_FLAGS = -Wall -Wextra -std=c++17

all: $(TARGET_EXEC_SERVER) $(TARGET_EXEC_CLIENT)

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

$(TARGET_EXEC_CLIENT): $(OBJS_CLIENT)
	$(CXX) $(OBJS_CLIENT) -o $@ $(LDFLAGS) $(LIBS)

# c source
$(BUILD_DIR_CLIENT)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR_CLIENT)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS_CLIENT) $(CXX_FLAGS) -c $< -o $@ $(LIBS)


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR_SERVER)
	$(RM) -r $(BUILD_DIR_CLIENT)
	rm screen-worms-server
	rm screen-worms-client

-include $(DEPS_SERVER) $(DEPS_CLIENT)

MKDIR_P = mkdir -p