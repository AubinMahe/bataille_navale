CC             = gcc
CC_WIN64       = x86_64-w64-mingw32-gcc-posix
OPTIM_OR_DEBUG = -O0 -g3
CFLAGS         = -std=c11 -D_POSIX_C_SOURCE=20210601 $(OPTIM_OR_DEBUG) -pedantic -pedantic-errors -Wall -Wextra -Werror
CFLAGS        += -Wconversion -Wcast-align -Wcast-qual -Wdisabled-optimization -Wlogical-op -Wmissing-declarations -Wundef
CFLAGS        += -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion -Wswitch-default -Wwrite-strings
CFLAGS        += -Wfloat-equal -fmessage-length=0 -pthread -MMD -MP
CFLAGS_WIN64   = -I ncurses-for-mingw64/include/ncursesw -I ncurses-for-mingw64/include -D_WIN32_WINNT=0x0601 $(CFLAGS)
LDFLAGS        = -lncurses -pthread
LDFLAGS_WIN64  = -static-libgcc -L ncurses-for-mingw64/lib -lncursesw -lws2_32

SRCS =\
 src/bataille_navale.c\
 src/errors.c\
 src/ia.c\
 src/main.c\
 src/protocole.c\
 src/traces.c\
 src/utils.c

OBJS       = $(SRCS:%.c=BUILD/%.o)
DEPS       = $(SRCS:%.c=BUILD/%.o.d)
OBJS_WIN64 = $(SRCS:%.c=BUILD/WIN64/%.o)
DEPS_WIN64 = $(SRCS:%.c=BUILD/WIN64/%.o.d)

GAME       = BUILD/bataille_navale
GAME_WIN64 = BUILD/bataille_navale.exe

.PHONY: all clean test

all: $(GAME) $(GAME_WIN64)

clean:
	rm -fr BUILD

test: $(GAME)
	./lance-les-deux.sh $(GAME)

test-win64: $(GAME_WIN64)
	export TERM=xterm && wine $(GAME_WIN64) --nom=Aubin --reseau=127.0.0.1:2416/127.0.0.1:2417 --journal=/tmp/bataille_navale-aubin.txt

$(GAME): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(GAME_WIN64): $(OBJS_WIN64)
	$(CC_WIN64) $(OBJS_WIN64) -o $@ $(LDFLAGS_WIN64)
	cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll BUILD
	cp ncurses-for-mingw64/bin/libncursesw6.dll        BUILD

BUILD/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC) -c $(CFLAGS) -MF$@.d -MT$@ $< -o $@

BUILD/WIN64/%.o: %.c
	@mkdir -p $$(dirname $@)
	$(CC_WIN64) -c $(CFLAGS_WIN64) -MF$@.d -MT$@ $< -o $@

-include $(DEPS)
-include $(DEPS_WIN64)
