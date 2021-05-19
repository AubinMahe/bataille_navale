CC             = gcc
CC_WIN64       = x86_64-w64-mingw32-gcc-posix
OPTIM_OR_DEBUG = -O0 -g3
CFLAGS         = -std=c11 -D_POSIX_C_SOURCE=20210601 $(OPTIM_OR_DEBUG) -pedantic -pedantic-errors -Wall -Wextra -Werror
CFLAGS        += -Wconversion -Wcast-align -Wcast-qual -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wundef
CFLAGS        += -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion -Wswitch-default -Wwrite-strings
CFLAGS        += -Wfloat-equal -fvisibility=hidden -fmessage-length=0 -pthread -MMD -MP
CFLAGS_WIN64   = -I ncurses-for-mingw64/include/ncursesw -I ncurses-for-mingw64/include -D_WIN32_WINNT=0x0601 $(CFLAGS)
LDFLAGS        = -lncurses -pthread
LDFLAGS_LIB    = -pthread
LDFLAGS_WIN64  = -static-libgcc -L ncurses-for-mingw64/lib -lncursesw -lws2_32

SRCS_LIB =\
 src/bataille_navale.c\
 src/errors.c\
 src/ia.c\
 src/protocole.c\
 src/traces.c\
 src/utils.c

SRCS =\
 src/ihm.c\
 src/main.c

OBJS           = $(SRCS:%.c=BUILD/%.o)
OBJS_LIB       = $(SRCS_LIB:%.c=BUILD/LIB/%.o)
OBJS_WIN64     = $(SRCS:%.c=BUILD/WIN64/%.o)
OBJS_WIN64_LIB = $(SRCS_LIB:%.c=BUILD/WIN64/LIB/%.o)

DEPS           = $(SRCS:%.c=BUILD/%.o.d)
DEPS_LIB       = $(SRCS_LIB:%.c=BUILD/LIB/%.o.d)
DEPS_WIN64     = $(SRCS:%.c=BUILD/WIN64/%.o.d)
DEPS_WIN64_LIB = $(SRCS_LIB:%.c=BUILD/WIN64/LIB/%.o.d)
               
APP_NAME       = bataille_navale
GAME           = BUILD/$(APP_NAME)
GAME_LIB       = BUILD/lib$(APP_NAME).so
GAME_WIN64     = BUILD/$(APP_NAME).exe
GAME_WIN64_LIB = BUILD/$(APP_NAME).dll

.PHONY: all clean test

all: $(GAME) $(GAME_LIB) $(GAME_WIN64)

clean:
	rm -fr BUILD

usage: $(GAME)
	./bn.sh $(GAME) usage ; true

test-ia: $(GAME)
	./bn.sh $(GAME) ia

test-deux-joueurs: $(GAME)
	./bn.sh $(GAME) test

test-wine: $(GAME_WIN64)
	./bn.sh $(GAME) test-wine

externe: $(GAME)
	./bn.sh $(GAME) externe

$(GAME_LIB): $(OBJS_LIB)
	$(CC) $(OBJS_LIB) -shared -o $@ $(LDFLAGS)

$(GAME): $(GAME_LIB) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) -L BUILD -l$(APP_NAME)

$(GAME_WIN64_LIB): $(OBJS_WIN64_LIB)
	$(CC_WIN64) $(OBJS_WIN64_LIB) -shared -o $@ $(LDFLAGS_WIN64)

$(GAME_WIN64): $(GAME_WIN64_LIB) $(OBJS_WIN64)
	$(CC_WIN64) $(OBJS_WIN64) -o $@ $(LDFLAGS_WIN64) -L BUILD -l$(APP_NAME)
	cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll BUILD
	cp ncurses-for-mingw64/bin/libncursesw6.dll        BUILD

BUILD/LIB/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC) -c -fpic -DBUILDING_DLL $(CFLAGS) -MF$@.d -MT$@ $< -o $@

BUILD/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC) -c $(CFLAGS) -MF$@.d -MT$@ $< -o $@

BUILD/WIN64/LIB/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC_WIN64) -c -DBUILDING_DLL $(CFLAGS_WIN64) -MF$@.d -MT$@ $< -o $@

BUILD/WIN64/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC_WIN64) -c $(CFLAGS_WIN64) -MF$@.d -MT$@ $< -o $@

-include $(DEPS)
-include $(DEPS_LIB)
-include $(DEPS_WIN64)
-include $(DEPS_WIN64_LIB)
