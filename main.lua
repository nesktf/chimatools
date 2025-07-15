local fnl = require("fennel").install()
fnl.path = fnl.path .. ";./?/init.fnl;./?/?.fnl;./?.fnl"
fnl.dofile("main.fnl")
