libarsc_objects :=
libarsc_objects += blob.o
libarsc_objects += common.o
libarsc_objects += filemap.o

binary := arsc

headers :=
headers += blob.h
headers += common.h
headers += filemap.h

libarsc = libarsc.a
objects := $(binary).o $(libarsc_objects)
deps := $(objects:.o=.d)

CC := clang
CFLAGS := -Wall -Wextra -ggdb -O0
CFLAGS += -DDEBUG

LD := $(CC)
LDFLAGS := $(CFLAGS)
LIBS := $(libarsc)

ifndef V
	QUIET_DEP = @echo "    DEP $@";
	QUIET_CC = @echo "    CC $@";
	QUIET_LD = @echo "    LINK $@";
	QUIET_AR = @echo "    AR $@";
endif

%.d: %.c
	$(QUIET_DEP)$(CC) $(CFLAGS) -MM $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

all: $(binary)

$(libarsc): $(libarsc_objects)
	$(QUIET_AR)$(RM) $@ && $(AR) rcs $@ $^

$(binary): $(binary).o $(LIBS)
	$(QUIET_LD)$(LD) $(LDFLAGS) -o $@ $^

test:
	$(MAKE) -C t all

clean:
	$(MAKE) -C t clean
	$(RM) $(deps)
	$(RM) $(objects)
	$(RM) $(LIBS)
	$(RM) $(binary)

-include $(deps)
