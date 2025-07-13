package.path = package.path .. ";lib/?.lua;lib/?/init.lua;lib/?/?.lua;src/?.lua;src/?/init.lua;src/?/?.lua"

local fnl = require("lib.fennel").install()
fnl.path = fnl.path .. ";lib/?.fnl;lib/?/init.fnl;lib/?/?.fnl;src/?.fnl;src/?/init.fnl;src/?/?.fnl"
fnl.dofile(arg[1])
