FNLC := fennel -c
BUILDDIR := build/chimatools/
ROCKSPEC := chimatools-dev-1.rockspec
SRCFILES := $(wildcard chimatools/*.fnl)
LUAFILES := $(subst .fnl,.lua,$(SRCFILES))

.PHONY: run build clean install

run:
	@luajit main.lua

build: $(LUAFILES)

install: $(LUAFILES)
	luarocks --lua-version=5.1 make --local $(ROCKSPEC)

%.lua: %.fnl $(BUILDDIR)
	$(FNLC) $< > build/$@

$(BUILDDIR):
	mkdir -p $@

clean:
	rm -rf build/
