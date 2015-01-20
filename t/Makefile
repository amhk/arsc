manifests := $(shell find . -type f -name AndroidManifest.xml -printf '%P\n')
apks := $(patsubst %/AndroidManifest.xml,%.apk,$(manifests))
deps := $(apks:.apk=.d)
arscs := $(apks:.apk=.arsc)

ifndef V
	QUIET_DEP = @echo "    DEP $@";
	QUIET_AAPT = @echo "    AAPT $@";
	QUIET_UNZIP = @echo "    UNZIP $@";
endif

%.d: %/AndroidManifest.xml
	$(QUIET_DEP)echo -n "$(patsubst %/AndroidManifest.xml,%,$<).apk $@ : $(shell find $(patsubst %/AndroidManifest.xml,%,$<) -type f | tr '\n' ' ')" > $@

%.apk: %/AndroidManifest.xml
	$(QUIET_AAPT)aapt package -M $< -S $(patsubst %/AndroidManifest.xml,%/res,$<) -F $@ -f

%.arsc: %.apk
	$(QUIET_UNZIP)unzip -p $< resources.arsc > $@

all: $(apks) $(arscs)

clean:
	$(RM) $(apks)
	$(RM) $(deps)
	$(RM) $(arscs)

-include $(deps)
