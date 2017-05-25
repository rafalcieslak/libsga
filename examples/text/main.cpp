#include <sga.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include "../common/common.hpp"

#include <map>
#include <iostream>

FT_Library ft;
FT_Face face;
unsigned int fontsize = 162;

const std::string font_path = EXAMPLE_DATA_DIR "open_sans/OpenSans-Regular.ttf";

const std::string text = R"(
This is some text.
)";

int main(){
  sga::init();

  FT_Init_FreeType(&ft);
  int error = FT_New_Face(ft, font_path.c_str(), 0, &face);
  if(error){
    std::cout << "Unable to open font " << font_path << std::endl;
    return -1;
  }

  FT_Set_Pixel_Sizes(face, 0, fontsize);

  struct GlyphData{
    FT_GlyphSlot glyph;
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
      image = sga::Image::create(w, h, 1);
      std::vector<uint8_t> buffer(bitmap.buffer, bitmap.buffer + w*h);
      image->putData(buffer);
      //std::cout << "Glyph " << c << " w: " << w << ", h: " << h << ", p: " << bitmap.pitch << std::endl;
    }
    glyphs[c] = {glyph, image};
  }


  auto window = sga::Window::create(800, 600, "Teapot");
  window->setFPSLimit(60);
  
  auto textShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 coords = sgaViewportCoords;
      float q = texture(glyph, coords).r;
      outColor = vec4(q,q,q,1);
    }
  )");

  textShader->addSampler("glyph");
  textShader->addOutput(sga::DataType::Float4, "outColor");
  
  auto textProgram = sga::Program::createAndCompile(textShader);
  auto textPipeline = sga::FullQuadPipeline::create();

  textPipeline->setProgram(textProgram);
  textPipeline->setTarget(window);

  textPipeline->setViewport(50,50,300,350);
  
  while(window->isOpen()){
    textPipeline->clear();

    char c = '%';
    auto it = glyphs.find(c);
    if(it != glyphs.end()){
      const GlyphData& glyph = glyphs[c];
      
      textPipeline->setSampler("glyph", glyph.image, sga::SamplerInterpolation::Nearest);
      textPipeline->drawFullQuad();
    }
    window->nextFrame();
  }
  sga::terminate();
}
