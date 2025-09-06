ifneq ("$(wildcard .env)","")
include .env
export
endif

CC := gcc
CFLAGS_DEBUG := -O0 -Wall -Wextra -Werror -DGLFW_INCLUDE_NONE -fno-omit-frame-pointer
CFLAGS_RELEASE := -D NDEBUG -O3 -Wall -Wextra -Werror -DGLFW_INCLUDE_NONE
SRCS := vk2c.c vk2c_native.c
BIN := vk2c

GLFW_ROOT := deps/GLFW
VK_ROOT := deps/Vulkan/1.4.321.0
INCLUDES_FLAGS := -I $(CURDIR)/$(GLFW_ROOT)/include -I $(CURDIR)/$(VK_ROOT)/include
LIB_DIRS := -L$(GLFW_ROOT)/lib -L$(VK_ROOT)/lib
LIBS := -lglfw3 -lvulkan -framework Cocoa -framework OpenGL -framework IOKit

.PHONY: all clean release
all:
	export DYLD_LIBRARY_PATH="$(CURDIR)/$(VK_ROOT)/lib:$DYLD_LIBRARY_PATH"
	$(CC) $(CFLAGS_DEBUG) $(INCLUDES_FLAGS) $(LIB_DIRS) -o $(BIN) $(SRCS) $(LIBS)

release: $(BIN)
	$(CC) $(CFLAGS_RELEASE) $(INCLUDES_FLAGS) $(LIB_DIRS) -o $(BIN) $(SRCS) $(LIBS)

clean:
	rm -f $(BIN)
