libarsc_objects :=
libarsc_objects += blob.o
libarsc_objects += cmds/dump.o
libarsc_objects += cmds/test.o
libarsc_objects += common.o
libarsc_objects += filemap.o
libarsc_objects += options.o

binary := arsc

headers :=
headers += blob.h
headers += cmds.h
headers += common.h
headers += filemap.h
headers += options.h

libarsc = libarsc.a
objects := $(binary).o $(libarsc_objects)
deps := $(objects:.o=.d)

manifests := $(shell find t -type f -name AndroidManifest.xml -print)
apks := $(patsubst %/AndroidManifest.xml,%.apk,$(manifests))
deps += $(apks:.apk=.d)
arscs := $(apks:.apk=.arsc)

CC := clang
CFLAGS := -Wall -Wextra -I. -ggdb -O0
CFLAGS += -DDEBUG

LD := $(CC)
LDFLAGS := $(CFLAGS)
LIBS := $(libarsc)

ifndef V
	QUIET_DEP = @echo "    DEP $@";
	QUIET_CC = @echo "    CC $@";
	QUIET_LD = @echo "    LINK $@";
	QUIET_AR = @echo "    AR $@";
	QUIET_AAPT = @echo "    AAPT $@";
	QUIET_UNZIP = @echo "    UNZIP $@";
endif

%.d: %.c
	$(QUIET_DEP)$(CC) $(CFLAGS) -MM $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

%.d: %/AndroidManifest.xml
	$(QUIET_DEP)echo -n "$(patsubst %/AndroidManifest.xml,%,$<).apk $@ : $(shell find $(patsubst %/AndroidManifest.xml,%,$<) -type f | tr '\n' ' ')" > $@

%.apk: %/AndroidManifest.xml
	$(QUIET_AAPT)aapt package -M $< -S $(patsubst %/AndroidManifest.xml,%/res,$<) -F $@ -f

%.arsc: %.apk
	$(QUIET_UNZIP)unzip -p $< resources.arsc > $@

all: $(binary)

$(libarsc): $(libarsc_objects)
	$(QUIET_AR)$(RM) $@ && $(AR) rcs $@ $^

$(binary): $(binary).o $(LIBS)
	$(QUIET_LD)$(LD) $(LDFLAGS) -o $@ $^

.PHONY: test
test: $(binary) $(apks) $(arscs)

clean:
	$(RM) $(deps)
	$(RM) $(objects)
	$(RM) $(LIBS)
	$(RM) $(binary)
	$(RM) $(apks)
	$(RM) $(arscs)

-include $(deps)
