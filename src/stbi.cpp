#include <stbi.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// A bug in GCC < 6 causes linker failures wrt __cpu_model if SIMD is enabled.
#define STBI_NO_SIMD

#define STB_IMAGE_STATIC
#include "../external/stb_image.h"
#include "../external/stb_image_write.h"
