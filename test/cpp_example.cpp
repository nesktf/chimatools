#include "../include/chimatools/chimatools.hpp"

#include <iostream>

int main() {
  try {
    // chima::context is the only owning RAII resource by default
    // Use chima::context_view if you need a non-owning wrapper
    chima::context chima;

    chima::image mari(chima, CHIMA_DEPTH_8U, "./test/data/witchmacs.png");
    chima::scoped_resource mari_scope(chima, mari);

    chima::image blank(chima, 512, 512, 4, CHIMA_DEPTH_8U, chima::color(1.f, 1.f, 1.f));
    chima::scoped_resource blank_scope(chima, blank);
    blank.composite(mari, 0, 0);

    chima::image chimata(chima, CHIMA_DEPTH_8U, "./test/data/chimata.png");
    chima::scoped_resource chimata_scope(chima, chimata);

    chima::image_anim keiki(chima, "./test/data/keiki_hello.gif");
    chima::scoped_resource keiki_scope(chima, keiki);

    chima::sheet_data data(chima);
    chima::scoped_resource data_scope(chima, data);
    data.add_anim(keiki, "keiki_hello").add_image(chimata, "chimata");

    static constexpr chima_u32 padding = 0;
    chima::spritesheet spritesheet(chima, data, padding, chima::color(1.f, 1.f, 1.f));
    chima::scoped_resource spritesheet_scope(chima, spritesheet);

#ifdef CHIMA_NO_DOWNCASTING
    const auto& atlas = spritesheet.get().atlas;
    const auto res =
      chima_write_image(chima, &atlas, CHIMA_FILE_FORMAT_PNG, "./test/data/test_sheet.png");
    if (res != CHIMA_NO_ERROR) {
      throw chima::error(res);
    }
#else
    spritesheet.atlas().write(chima, CHIMA_FILE_FORMAT_PNG, "./test/data/test_sheet.png");
#endif
    spritesheet.write(chima, CHIMA_FILE_FORMAT_PNG, "./test/data/test_sheet.chima");

  } catch (chima::error& err) {
    std::cout << "CHIMA ERROR (" << err.code() << "): " << err.what() << "\n";
    return 1;
  }
  return 0;
}
