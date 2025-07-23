PROJNAME  		:= chimatools
BUILD_DIR			?= build
INCLUDE_DIR 	:= include/chimatools
LUA_BUILD_DIR := $(BUILD_DIR)/lua

CC 			:= gcc
AR 			:= ar rcs
SRC_DIR	:= src
CFLAGS 	:= -std=c11 -Wall -Wextra -I$(SRC_DIR) -I$(INCLUDE_DIR)
LDFLAGS := -lm

LIB_CFLAGS	:= $(CFLAGS) -fPIC
LIB_SHARED	:= $(BUILD_DIR)/lib$(PROJNAME).so
LIB_STATIC 	:= $(BUILD_DIR)/lib$(PROJNAME).a

BIN_CFLAGS 	:= $(CFLAGS)
BIN_LDFLAGS := -L$(BUILD_DIR) -l$(PROJNAME)

LIB_SRCS	:= $(SRC_DIR)/chimatools.c
LIB_OBJS  := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/lib_%.o, $(LIB_SRCS))

BIN_SRCS 	:= $(SRC_DIR)/example.c
BIN_OBJS	:= $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/bin_%.o, $(BIN_SRCS))
BIN_EXE		:= $(BUILD_DIR)/chimaexample

LUA_ROCKSPEC 	:= $(PROJNAME)-dev-1.rockspec
FNLC					:= fennel -c
FNL_SRC_DIR 	:= fnl

FNL_LIB_SRCS 	:= $(wildcard $(FNL_SRC_DIR)/$(PROJNAME)/*.fnl)
LUA_LIB_SRCS	:= $(patsubst $(FNL_SRC_DIR)/$(PROJNAME)/%.fnl, $(LUA_BUILD_DIR)/$(PROJNAME)/%.lua, $(FNL_LIB_SRCS))

FNL_BIN_SRCS	:= $(wildcard $(FNL_SRC_DIR)/*.fnl)
LUA_BIN_SRCS	:= $(patsubst $(FNL_SRC_DIR)/%.fnl, $(LUA_BUILD_DIR)/%, $(FNL_BIN_SRCS))

MIME_XML := xdg/chima-asset.xml

.PHONY: shared lib bin lua rock clean install all

all: lib shared bin lua

install: shared
	mkdir -p /usr/include/chimatools/
	cp -r $(INCLUDE_DIR) /usr/include/
	cp $(LIB_INCL) /usr/include/chimatools/
	cp $(LIB_SHARED) /usr/lib/x86_64-linux-gnu/
	cp $(MIME_XML) /usr/share/mime/packages/
	update-mime-database /usr/share/mime/

shared: $(LIB_SHARED)

lib: $(LIB_STATIC)

$(LIB_SHARED): $(LIB_OBJS)
	$(CC) $^ $(LDFLAGS) -shared -o $@

$(LIB_STATIC): $(LIB_OBJS)
	$(AR) $@ $^

$(BUILD_DIR)/lib_%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/
	$(CC) $(LIB_CFLAGS) -c $< -o $@

$(BUILD_DIR)/:
	mkdir -p $(BUILD_DIR)

bin: $(BIN_EXE)

$(BIN_EXE): $(BIN_OBJS) $(LIB_SHARED)
	$(CC) $^ $(BIN_LDFLAGS) -o $@

$(BUILD_DIR)/bin_%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/
	$(CC) $(BIN_CFLAGS) -c $< -o $@

rock: lua
	luarocks --lua-version=5.1 make --local $(LUA_ROCKSPEC)

lua: $(LUA_BIN_SRCS) $(LUA_LIB_SRCS)

$(LUA_BUILD_DIR)/%: $(FNL_SRC_DIR)/%.fnl | $(LUA_BUILD_DIR)/
	echo "#!/usr/bin/luajit" > $@
	$(FNLC) $< >> $@
	chmod +x "$@"

$(LUA_BUILD_DIR)/$(PROJNAME)/%.lua: $(FNL_SRC_DIR)/$(PROJNAME)/%.fnl | $(LUA_BUILD_DIR)/
	$(FNLC) $< > $@

$(LUA_BUILD_DIR)/:
	mkdir -p $(LUA_BUILD_DIR)/$(PROJNAME)

clean:
	rm -rf $(BUILD_DIR)
