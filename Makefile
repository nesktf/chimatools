LUAINT := luajit
LUAMAIN := src/main.lua
FNLMAIN := src/chimatiler.fnl

.PHONY: run
run: $(LUAMAIN)
	@$(LUAINT) $(LUAMAIN) $(FNLMAIN)
