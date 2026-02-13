# chimatools
![chimatools](chimatools.png) 

Game engine asset management library for C11 and C++17, with LuaJIT bindings.

## Assets
The library provides tools to load and create `.chima` asset files. These are optimized to be
loaded easily in my custom made graphics engine [ShOGLE](https://github.com/nesktf/shogle).

Some of the things that the library support are:
- Basic image manipulation (loading, writting, compositing).
- Generation and loading of spritesheet assets.

## API
The entire library interface is located in the main header file
[chimatools.h](./include/chimatools/chimatools.h).

The library doesn't use any global state, in order to use the API you need to create
a `chima_context` object to hold an allocator and some other options.

Most structs are accesible by the user, there are only a handful of opaque handles.
However, you should use the construction and destruction functions when available (at least for
asset objects) since objects may be allocated using custom allocation functions
(check the `chima_alloc` struct for more info.).

```c
#include <chimatools/chimatools.h>
#include <stdio.h>

void my_chima_func(void) {
  chima_result res;
  chima_context chima;
  res = chima_create_context(&chima, NULL);
  if (res) {
    printf("%s\n", chima_error_string(res));
    return;
  }

  chima_spritesheet sheet;
  res = chima_load_spritesheet(chima, &sheet, "./atlas.chima");
  if (res) {
    printf("%s\n", chima_error_string(res));
    return;
  }
  // ... do things

  chima_destroy_spritesheet(chima, &sheet);
  chima_destroy_context(&chima);
}
``` 

For the C++ interface, there is a single header [chimatools.hpp](./include/chimatools/chimatools.hpp)
where everything is defined inline. The minimum supported version is C++17.

All classes and functions are inside the `chima` namespace. Every single class is just a wrapper
for C structs. We use the same naming conventions but without the `chima_` prefix from the C API.

Most bjects do NOT use RAII by default. If you want to destroy things you should use
the `destroy` static member functions for your objects or the `chima::scoped_resource` reference
wrapper for traditional RAII. The only exception is the `chima::context`; it is RAII compatible
by default (since most of the time you only need one).

Also, objects use both a traditional throwing constructor model and a non-thowing named
constructor model using return values and `std::optional` (not `std::expected` since we are
using C++17).
```cpp
#include <chimatools/chimatools.hpp>
#include <iostream>

void my_chima_func() {
  chima::context chima; // might throw
  chima::spritesheet sheet(chima, "./atlas.chima"); // might throw
  chima::scoped_resource sheet_scope(chima, sheet);

  chima::error err;
  auto other_sheet = chima::spritesheet::load(chima, "./atlas.chima", &err); // will not throw
  if (!other_sheet) { // `std::optional<chima::spritesheet>`
    std::cout << err.what() << "\n";
    return;
  }
  // ... do things
  chima::spritesheet::destroy(chima, other_sheet.value());
}
// `chima::scoped_resource` destroys `sheet`
// `chima::context` destroys itself
```

You can check the [examples](./examples) folder for some extra examples.

## LuaJIT bindings
The same functions and data types from C are available when via LuaJIT FFI binds. The only
difference is that all objects are managed by the garbage collector, so there is no need to call
destructors manually.
```lua
local chima = require('chimatools')
local ctx = chima.context.new()

local sheet = chima.spritesheet.load(ctx, "./atlas.chima")
-- ... do things
```

If, for some reason, you need to access the raw FFI namespace, the library FFI object is
exposed at `chima._lib`

## Building
You can either install the library on your system (as a shared library) or include it in your
project CMakeLists as a shared or static library.

Keep in mind that the library is meant to be used in a Linux based OS (at least for now).
```sh
$ cmake -B build_shared -DCMAKE_BUILD_TYPE=Release -DCHIMA_SHARED_BUILD=1 # shared build
$ make -C build_shared -j$(nproc)
$ sudo make -C build_shared install
$ cmake -B build_static -DCMAKE_BUILD_TYPE=Release # static build
$ make -C build_static -j$(nproc)
```

In your CMake project
```cmake
cmake_minimum_required(VERSION 3.10)
project(my_funny_project CXX C)
# ...
# then either
add_subdirectory(/path/to/chimatools)
target_include_directories(/path/to/chimatools/include/)
# or 
find_package(chimatools REQUIRED)
target_include_directories(${CHIMATOOLS_INCLUDE_DIRS})
# then finally
target_link_libraries(${PROJECT_NAME} chimatools)
```

To compile the Lua API, just run `build_lua.sh`. You need to have the `fennel` compiler installed
and (optionally) `luarocks` for installation.

## External libraries
- Modified [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) 
- Modified [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h)
- [stb_rect_pack](https://github.com/nothings/stb/blob/master/stb_rect_pack.h)
