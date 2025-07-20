package = "chimatools"
version = "dev-1"

source = {
  url = "git://github.com/nesktf/chimatools.git"
}

description = {
  summary = "LuaJIT FFI bindings for chimatools",
  license = "MIT",
  maintainer = "nesktf <nesktf@proton.me>"
}

dependencies = {
  "lua == 5.1",
  "fennel",
}

build = {
  type = "builtin",
  modules = {
    ["chimatools"] = "build/lua/chimatools/init.lua",
    ["chimatools.lib"] = "build/lua/chimatools/lib.lua",
    ["chimatools.sprite"] = "build/lua/chimatools/sprite.lua",
  },
  -- Disabled until i manage to instal luajit script binaries
  -- install = {
  --   bin = { "build/lua/chimasprite"}
  -- }
}
