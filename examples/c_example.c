#include "../include/chimatools/chimatools.h"
#include <assert.h>
#include <stdio.h>

#define BREAK() __asm("int3")
#define checkometoda() printf("%i:%s\n", __LINE__, chima_error_string(res)); assert(!res)

static void print_sprite_transf(chima_sprite const* sprite, chima_u32 w, chima_u32 h) {
  chima_uv_transf transf = chima_calc_uv_transform(w, h,
                                                   sprite->rect, CHIMA_UV_FLAG_FLIP_Y);
  printf("(x: %d, w: %d) -> (%f %f)\n", sprite->rect.x, sprite->rect.width,
         transf.x_lin, transf.x_con);
  printf("(y: %d, h: %d) -> (%f %f)\n", sprite->rect.y, sprite->rect.height,
         transf.y_lin, transf.y_con);
}

int main() {
  chima_context chima;
  chima_create_context(&chima, NULL);

  chima_result res;
  chima_image blank;
  const chima_color col = (chima_color){.r = 1.f, .g = 1.f, .b = 1.f, .a = 1.f};
  res = chima_gen_blank_image(chima, &blank, 512, 512, 4, CHIMA_DEPTH_8U, col);
  checkometoda();

  chima_image mari;
  res = chima_load_image(chima, &mari, CHIMA_DEPTH_8U, "./examples/data/witchmacs.png");
  checkometoda();

  res = chima_composite_image(&blank, &mari, 0, 0);
  checkometoda();

  chima_image_anim keiki;
  res = chima_load_image_anim(chima, &keiki, "./examples/data/keiki_hello.gif");
  checkometoda();

  chima_sheet_data data;
  res = chima_create_sheet_data(chima, &data);
  checkometoda();

  res = chima_sheet_add_image_anim(data, &keiki, "keiki_hello");
  checkometoda();

  chima_image chimata;
  res = chima_load_image(chima, &chimata, CHIMA_DEPTH_8U, "./examples/data/chimata.png");
  checkometoda();

  res = chima_sheet_add_image(data, &chimata, "chimata");
  checkometoda();

  chima_spritesheet sheet;
  res = chima_gen_spritesheet(chima, &sheet, data, 0, col);
  checkometoda();

  printf("sz: (w: %d, h: %d)\n", sheet.atlas.extent.width, sheet.atlas.extent.height);
  for (size_t i = 0; i < sheet.sprite_count; ++i) {
    print_sprite_transf(&sheet.sprites[i], sheet.atlas.extent.width, sheet.atlas.extent.height);
    printf("\n");
  }

  BREAK();

  chima_destroy_spritesheet(chima, &sheet);
  chima_destroy_sheet_data(data);
  chima_destroy_image_anim(chima, &keiki);
  chima_destroy_image(chima, &chimata);
  chima_destroy_image(chima, &mari);
  chima_destroy_image(chima, &blank);
  chima_destroy_context(chima);
}
