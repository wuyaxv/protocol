# Third-party components

This directory contains third-party single-file libraries, both released into
the public domain:

* **stb_image_write.h** — PNG image encoder.
  Sean Barrett / http://nothings.org/stb — Public domain (or MIT).
  https://github.com/nothings/stb

* **stb_truetype.h** — TrueType font rasterizer, used by the PNG renderer to
  draw text with modern system fonts.
  Sean Barrett / http://nothings.org/stb — Public domain (or MIT).
  https://github.com/nothings/stb

* **font8x8_basic.h** — 8x8 monochrome bitmap font (basic latin), used by the
  PNG renderer as a fallback when no system font is available.
  Daniel Hepper, based on IBM public domain VGA fonts — Public domain.
  https://github.com/dhepper/font8x8
  (Locally modified: glyph rows use `unsigned char` for C++11 narrowing
  compliance.)
