# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
CC=gcc

TARGET_EXEC ?= tc_server

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
TARGET_DIR ?= ./run

# Find all the C and C++ files we want to compile
SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c)

SRCS0 := \
		common.c \
		tc_insert.c \
		tc_remove.c \
		logit.c \
		nodeset.c \
		node_std.c \
		schema.c \
		dump1.c \
		header.c \
		teste_block.c \
		metadata_parse.c \
		metadata.c \
		mlnet.c \
		stats.c \
		pool.c \
		terminal.c \
		query.c \
		query_parse.c \
		query_out.c \
		register.c \
		class_cat.c \
		class_geo.c \
		contents.c \
		contents_stats.c \
		container_binlist.c \
		container_binlist_addonly.c \
		iserver.c \
		webserver_mg.c \
		mestrado_01.c \
		others/cJSON.c \
		others/cJSON_Utils.c \
		extra/str_to_epoch.c \
		others/map.c \
		others/mongoose.c \
		main.c


# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)

# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
#CPPFLAGS := $(INC_FLAGS) -MMD -MP

CPPFLAGS := $(INC_FLAGS) -MMD -MP -static

LDFLAGS	 := -lm	 -l:libtcmalloc_minimal.so.4

# The final build step.
$(TARGET_DIR)/$(TARGET_EXEC): $(OBJS)
	mkdir -p $(TARGET_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
#$(BUILD_DIR)/%.cpp.o: %.cpp
#	#mkdir -p $(dir $@)
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
