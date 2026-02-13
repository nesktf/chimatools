#include "../include/chimatools/chimatools.hpp"

#include <iostream>

static constexpr bool use_noexcept = true;
static constexpr chima_u32 padding = 0;

[[noreturn]] void die(const chima::error& err) {
  std::cout << "CHIMA ERROR (" << err.code() << "): " << err.what() << "\n";
  std::exit(1);
}

void run_thing() {
  // chima::context is the only owning RAII resource by default
  // Use chima::context_view if you need a non-owning wrapper
  chima::context chima;

  chima::image mari(chima, CHIMA_DEPTH_8U, "./examples/data/witchmacs.png");
  chima::scoped_resource mari_scope(chima, mari);

  chima::image blank(chima, 512, 512, 4, CHIMA_DEPTH_8U, chima::color(1.f, 1.f, 1.f));
  chima::scoped_resource blank_scope(chima, blank);
  blank.composite(mari, 0, 0);

  chima::image chimata(chima, CHIMA_DEPTH_8U, "./examples/data/chimata.png");
  chima::scoped_resource chimata_scope(chima, chimata);

  chima::image_anim keiki(chima, "./examples/data/keiki_hello.gif");
  chima::scoped_resource keiki_scope(chima, keiki);

  chima::sheet_data data(chima);
  chima::scoped_resource data_scope(chima, data);
  data.add_anim(keiki, "keiki_hello").add_image(chimata, "chimata");

  chima::spritesheet spritesheet(chima, data, padding, chima::color(1.f, 1.f, 1.f));
  chima::scoped_resource spritesheet_scope(chima, spritesheet);

#ifdef CHIMA_NO_DOWNCASTING
  const auto& atlas = spritesheet.get().atlas;
  const auto res =
    chima_write_image(chima, &atlas, CHIMA_FILE_FORMAT_PNG, "./examples/data/test_sheet.png");
  if (res != CHIMA_NO_ERROR) {
    throw chima::error(res);
  }
#else
  spritesheet.atlas().write(chima, CHIMA_FILE_FORMAT_PNG, "./examples/data/test_sheet.png");
#endif
  spritesheet.write(chima, CHIMA_FILE_FORMAT_PNG, "./examples/data/test_sheet.chima");
}

void run_thing_noexcept() noexcept {
  // noexcept named constructor versions returning a `std::optional` wrapped value.
  // `err` is optional.
  chima::error err;
  auto chima = chima::context::create(nullptr, &err);
  if (!chima) {
    die(err);
  }

  auto mari = chima::image::load(*chima, CHIMA_DEPTH_8U, "./examples/data/witchmacs.png", &err);
  if (!mari) {
    die(err);
  }
  chima::scoped_resource mari_scope(*chima, *mari);

  auto blank = chima::image::make_blank(*chima, 512, 512, 4, CHIMA_DEPTH_8U,
                                        chima::color(1.f, 1.f, 1.f), &err);
  if (!blank) {
    die(err);
  }
  chima::scoped_resource blank_scope(*chima, *blank);
  blank->composite(*mari, 0, 0, &err);
  if (err) {
    die(err);
  }

  auto chimata = chima::image::load(*chima, CHIMA_DEPTH_8U, "./examples/data/chimata.png", &err);
  if (!chimata) {
    die(err);
  }
  chima::scoped_resource chimata_scope(*chima, *chimata);

  auto keiki = chima::image_anim::load(*chima, "./examples/data/keiki_hello.gif", &err);
  if (!keiki) {
    die(err);
  }
  chima::scoped_resource keiki_scope(*chima, *keiki);

  auto data = chima::sheet_data::create(*chima, &err);
  if (!data) {
    die(err);
  }
  chima::scoped_resource data_scope(*chima, *data);
  data->add_anim(*keiki, "keiki_hello", &err);
  if (err) {
    die(err);
  }
  data->add_image(*chimata, "chimata");
  if (err) {
    die(err);
  }

  auto spritesheet =
    chima::spritesheet::make_from_data(*chima, *data, padding, chima::color(1.f, 1.f, 1.f), &err);
  if (!spritesheet) {
    die(err);
  }
  chima::scoped_resource spritesheet_scope(*chima, *spritesheet);

#ifdef CHIMA_NO_DOWNCASTING
  const auto& atlas = spritesheet->get().atlas;
  const auto res =
    chima_write_image(*chima, &atlas, CHIMA_FILE_FORMAT_PNG, "./examples/data/test_sheet.png");
  if (res != CHIMA_NO_ERROR) {
    err = res;
    die(err);
  }
#else
  spritesheet->atlas().write(*chima, CHIMA_FILE_FORMAT_PNG, "./examples/data/test_sheet.png",
                             &err);
  if (err) {
    die(err);
  }
#endif
  spritesheet->write(*chima, CHIMA_FILE_FORMAT_PNG, "./examples/data/test_sheet.chima", &err);
  if (err) {
    die(err);
  }
}

int main() {
  if (use_noexcept) {
    run_thing_noexcept();
  } else {
    try {
      run_thing();
    } catch (chima::error& err) {
      std::cout << "CHIMA ERROR (" << err.code() << "): " << err.what() << "\n";
      return 1;
    }
  }
  std::cout << "done!!1!11\n";
  return 0;
}
