local fnl = require("fennel").install()
fnl.path = fnl.path .. ";lib/?.fnl;lib/?/init.fnl;lib/?/?.fnl;src/?.fnl;src/?/init.fnl;src/?/?.fnl"
fnl.dofile(arg[1])
