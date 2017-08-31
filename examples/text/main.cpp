#include <sga.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include "../common/common.hpp"

#include <map>
#include <iostream>

FT_Library ft;
FT_Face face;
unsigned int fontsize = 32;

const std::string font_path = EXAMPLE_DATA_DIR "open_sans/OpenSans-Regular.ttf";

int main(){
  sga::init();

  // Load font data.
  FT_Init_FreeType(&ft);
  int error = FT_New_Face(ft, font_path.c_str(), 0, &face);
  if(error){
    std::cout << "Unable to open font " << font_path << std::endl;
    return -1;
  }
  FT_Set_Pixel_Sizes(face, 0, fontsize);
  struct GlyphData{
    FT_Glyph_Metrics metrics;
    std::shared_ptr<sga::Image> image;
  };
  std::map<char, GlyphData> glyphs;
  for(unsigned char c = 0; c < 128; c++){
    error = FT_Load_Char(face, c, FT_LOAD_RENDER);
    if(error)
      continue;
    FT_GlyphSlot glyph = face->glyph;
    std::shared_ptr<sga::Image> image;
    if(glyph->bitmap.rows > 0){
      FT_Bitmap bitmap;
      FT_Bitmap_Init(&bitmap);
      FT_Bitmap_Convert(ft, &glyph->bitmap, &bitmap, 1);
      unsigned int w = bitmap.width, h = bitmap.rows;
      image = std::make_shared<sga::Image>(sga::Image(w, h, 1));
      std::vector<uint8_t> buffer(bitmap.buffer, bitmap.buffer + w*h);
      image->putData(buffer);
    }
    glyphs[c] = {glyph->metrics, image};
  }

  sga::Window window(800, 200, "Font");
  window.setFPSLimit(60);

  auto textShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      float q = texture(glyph, sgaViewportCoords).r;
      vec3 color = vec3(0.4,0.8,0.2) * q;
      outColor = vec4(color,1);
    }
  )");

  textShader.addSampler("glyph");
  textShader.addOutput(sga::DataType::Float4, "outColor");

  auto textProgram = sga::Program::createAndCompile(textShader);
  sga::FullQuadPipeline textPipeline;

  textPipeline.setProgram(textProgram);
  textPipeline.setTarget(window);

  textPipeline.setViewport(50,50,300,350);

  int basex = 20;

  while(window.isOpen()){
    textPipeline.clear();

    char text_buffer[800];
    sprintf(text_buffer, R"(
This example demonstrates rendering text with
Freetype. It has been running for %.2fs.
Each frame the entire text is rendered again,
character by character. Average FPS: %.2fs.
)", sga::getTime(), window.getAverageFPS());

    #define REPEAT 1
    for(int i =0; i < REPEAT; i++){

    // Cursor coordinates;
    float x = basex, y = 0;

    for(char c : std::string(text_buffer)){
      auto it = glyphs.find(c);
      if(it != glyphs.end()){
        const GlyphData& glyph = glyphs[c];

        if(!isspace(c) && glyph.image){
          float charx = x + glyph.metrics.horiBearingX / 64.0f;
          float chary = y - glyph.metrics.horiBearingY / 64.0f;
          float charw = glyph.image->getWidth();
          float charh = glyph.image->getHeight();

          textPipeline.setViewport(charx, chary, charx + charw, chary + charh);
          textPipeline.setSampler("glyph", *glyph.image, sga::SamplerInterpolation::Nearest);
          textPipeline.drawFullQuad();
        }

        x += glyph.metrics.horiAdvance / 64.0f;
      }
      if(c == '\n'){
        y += face->size->metrics.height / 64.0f;
        x = basex;
      }

    }
    }
    window.nextFrame();
  }
  sga::terminate();
}
