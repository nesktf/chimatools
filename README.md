# chimatools
![chimatools](chimatools.png) 

Game engine asset packing library for C11, with LuaJIT bindings.

## Assets
The library provides tools to load and create `.chima` asset files. These are optimized to be
loaded easily in my custom made graphics engine [ShOGLE](https://github.com/nesktf/shogle).

Currently, the following are supported:
- Basic image manipulation (loading, writting, compositing)
- Spritesheets (generation and loading)

The following are planned to be implemented in the future:
- Bitmap font atlases
- 3D models
- 3D animations
- Serialized game state

## C API
The entire library interface is located in the main header file [chimatools.h](./src/chimatools.h).
The library doesn't use any global state, so in order to use the API you need to create
a `chima_context` object.

Most structs are accesible by the user, the only opaque data type is the `chima_context`.
However, at least for asset objects, you should use the construction and destruction functions
when available.
```c
#include <chimatools/chimatools.h>

chima_context chima;
chima_create_context(&chima, NULL);

chima_spritesheet sheet;
chima_load_spritesheet(chima, &sheet, "atlas.chima");
// .. do things

chima_destroy_spritesheet(&sheet);
chima_destroy_context(&chima);
``` 
If you want to use custom allocator functions, see the `chima_alloc` struct.

See the [src/example.c](./src/example.c) file for some examples.

## LuaJIT bindings
The same functions and data types from C are available when using the LuaJIT binds. The only
difference is that all objects are managed by the garbage collector, so there no need to call
destructors manually.
```lua
local chima = require('chimatools')
local ctx = chima.context.new()

local sheet = chima.spritesheet.load(ctx, "atlas.chima")
-- .. do things
```

If, for some reason, you need to access the raw FFI namespace, the library FFI object is
exposed at `chima._lib`

## Installation
Just run the makefile. The library is meant to be used in a Linux based OS (at least for now).
```sh
make lib # Compile the library
make bin # Compile the example
make lua # Compile the lua library
sudo make install # Install the C library
make rock # Install the luarocks package locally
```

## External libraries
- Modified [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) 
- Modified [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h)
- [stb_rect_pack](https://github.com/nothings/stb/blob/master/stb_rect_pack.h)
