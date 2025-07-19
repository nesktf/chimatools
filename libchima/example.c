#include "chimatools.h"
#include <assert.h>
#include <stdio.h>

int main() {
  chima_context chima;
  chima_create_context(&chima, NULL);

  chima_return ret;
  chima_image images[2];
  chima_anim anims[4];

  ret = chima_load_image(chima, images, "chimata", "res/chimata.png");
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_load_image(chima, images+1, "marisa", "res/marisa_emacs.png");
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_load_anim(chima, anims, "cino", "res/cino.gif");
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_load_anim(chima, anims+1, "mari_ass", "res/mariass.gif");
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  ret = chima_load_anim(chima, anims+2, "honk", "res/honk.gif");
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_load_anim(chima, anims+3, "rin_dance", "res/rin.gif");
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  chima_spritesheet sheet;
  chima_set_sheet_initial(chima, 2304);
  chima_create_spritesheet(chima, &sheet, 5,
                           images, CHIMA_ARR_SZ(images), anims, CHIMA_ARR_SZ(anims));

  const char asset_path[] = "res/chimaout.chima";
  printf("Writting sheet to %s\n", asset_path);
  chima_set_sheet_format(chima, CHIMA_FORMAT_PNG);
  chima_write_spritesheet(chima, asset_path, &sheet);

  chima_spritesheet readsheet;
  ret = chima_load_spritesheet(chima, "res/chimaout.chima", &readsheet);
  printf("%s\n", chima_error_string(ret));
  assert(ret == CHIMA_NO_ERROR);

  const char atlas_path[] = "res/chima_test.png";
  printf("Writting atlas to %s\n", atlas_path);
  chima_write_image(chima, &readsheet.atlas, atlas_path, CHIMA_FORMAT_PNG);

  for (size_t i = 0; i < CHIMA_ARR_SZ(images); ++i){
    chima_destroy_image(images+i);
  }
  for (size_t i = 0; i < CHIMA_ARR_SZ(anims); ++i) {
    chima_destroy_anim(anims+i);
  }
  chima_destroy_spritesheet(&sheet);
  chima_destroy_spritesheet(&readsheet);

  chima_destroy_context(chima);
}
