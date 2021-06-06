CC             := gcc
CC_WIN64       := x86_64-w64-mingw32-gcc-posix
OPTIM_OR_DEBUG := -O0 -g3 -ggdb3
OPTIM_OR_DEBUG := -O3 -g0
CFLAGS         := -std=c11 -D_POSIX_C_SOURCE=20210601 $(OPTIM_OR_DEBUG) -pedantic -pedantic-errors -Wall -Wextra -Werror
CFLAGS         += -Wconversion -Wcast-align -Wcast-qual -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wundef
CFLAGS         += -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion -Wswitch-default -Wwrite-strings
CFLAGS         += -Wfloat-equal -fvisibility=hidden -fmessage-length=0 -pthread -MMD -MP
CFLAGS_WIN64   := -D_WIN32_WINNT=0x0601 $(CFLAGS)
CFLAGS_TEST    := $(CFLAGS) -Isrc
LDFLAGS        := -lncurses -pthread
LDFLAGS_LIB    := -pthread
LDFLAGS_WIN64  := -lws2_32 -static

SRCS_LIB :=\
 src/bataille_navale.c\
 src/errors.c\
 src/ia.c\
 src/protocole.c\
 src/traces.c\
 src/utils.c

SRCS_LINUX :=\
 src/ihm_ncurses.c\
 src/main.c

SRCS_WIN64 :=\
 src/ihm_dos.c\
 src/main.c

SRCS_TEST :=\
 test/main.c

OBJS_LINUX_LIB    := $(SRCS_LIB:%.c=BUILD/LIB/%.o)
OBJS_LINUX        := $(SRCS_LINUX:%.c=BUILD/%.o)
OBJS_WIN64_LIB    := $(SRCS_LIB:%.c=BUILD/WIN64/LIB/%.o)
OBJS_WIN64        := $(SRCS_WIN64:%.c=BUILD/WIN64/%.o)
OBJS_TEST         := $(OBJS_LINUX_LIB) $(SRCS_TEST:%.c=BUILD/%.o)
                  
DEPS_LINUX_LIB    := $(SRCS_LIB:%.c=BUILD/LIB/%.o.d)
DEPS_LINUX        := $(SRCS_LINUX:%.c=BUILD/%.o.d)
DEPS_WIN64_LIB    := $(SRCS_LIB:%.c=BUILD/WIN64/LIB/%.o.d)
DEPS_WIN64        := $(SRCS_WIN64:%.c=BUILD/WIN64/%.o.d)
               
APP_NAME          := bataille_navale
TEST_LINUX        := BUILD/$(APP_NAME)-tests
GAME_LINUX        := BUILD/$(APP_NAME)
GAME_LINUX_LIB    := BUILD/lib$(APP_NAME).so
GAME_WIN64        := BUILD/$(APP_NAME).exe
GAME_WIN64_LIB    := BUILD/$(APP_NAME).dll
GAME_WIN64_IMPLIB := BUILD/lib$(APP_NAME).dll.a

.PHONY: all clean test

all: $(TEST_LINUX) $(GAME_LINUX_LIB) $(GAME_LINUX) $(GAME_WIN64_LIB) $(GAME_WIN64)

clean:
	rm -fr BUILD

usage: $(GAME_LINUX)
	./bn.sh $(GAME_LINUX) usage ; true

test-ia: $(GAME_LINUX)
	./bn.sh $(GAME_LINUX) ia

test-deux-joueurs: $(GAME_LINUX)
	./bn.sh $(GAME_LINUX) test

test-wine: $(GAME_LINUX) $(GAME_WIN64)
	./bn.sh $(GAME_LINUX) test-wine

externe: $(GAME_LINUX)
	./bn.sh $(GAME_LINUX) externe

$(TEST_LINUX): $(OBJS_TEST)
	$(CC) -o $@ $(OBJS_TEST) $(LDFLAGS)

$(GAME_LINUX_LIB): $(OBJS_LINUX_LIB)
	$(CC) -o $@ $(OBJS_LINUX_LIB) -shared $(LDFLAGS)

$(GAME_LINUX): $(GAME_LINUX_LIB) $(OBJS_LINUX)
	$(CC) -o $@ $(OBJS_LINUX) $(LDFLAGS) -L BUILD -l$(APP_NAME)

$(GAME_WIN64_LIB) $(GAME_WIN64_IMPLIB): $(OBJS_WIN64_LIB)
	$(CC_WIN64) -o $(GAME_WIN64_LIB) $(OBJS_WIN64_LIB) -shared $(LDFLAGS_WIN64) -Wl,--out-implib=$(GAME_WIN64_IMPLIB)

$(GAME_WIN64): $(GAME_WIN64_IMPLIB) $(OBJS_WIN64)
	$(CC_WIN64) -o $@ $(OBJS_WIN64) -L BUILD -l$(APP_NAME).dll $(LDFLAGS_WIN64)
	cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll BUILD

BUILD/LIB/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC) -c -fpic -DBUILDING_DLL $(CFLAGS) -MF$@.d -MT$@ $< -o $@

BUILD/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC) -c $(CFLAGS) -MF$@.d -MT$@ $< -o $@

BUILD/test/%.o: test/%.c
	@mkdir -p $$(dirname $@)
	$(CC) -c $(CFLAGS_TEST) -MF$@.d -MT$@ $< -o $@

BUILD/WIN64/LIB/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC_WIN64) -c -DBUILDING_DLL $(CFLAGS_WIN64) -MF$@.d -MT$@ $< -o $@

BUILD/WIN64/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC_WIN64) -c $(CFLAGS_WIN64) -MF$@.d -MT$@ $< -o $@

-include $(DEPS_LINUX)
-include $(DEPS_LINUX_LIB)
-include $(DEPS_WIN64)
-include $(DEPS_WIN64_LIB)
