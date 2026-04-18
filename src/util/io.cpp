#include "io.h"

#include <glm/common.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace io {

    void output_image(const std::string& filename,
        const uchar4* fb,
        const size_t width, const size_t height) {

        const int stride_in_bytes = width * sizeof(uchar4);

        if (stbi_write_png(
            filename.c_str(),
            (int)width, (int)height,
            4,
            reinterpret_cast<const void*>(fb),
            stride_in_bytes))
        {
            std::cout << "Image successfully saved as '" << filename << "'.\n";
            system(("start " + filename).c_str());
        }
        else {
            std::cerr << "An error occured while writing the PNG file.\n";
        }
    }

    std::string readTextFile(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("couldnt open file : " + path);

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

}