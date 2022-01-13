
#include <iostream>
#include <fstream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

int main (int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: view <data file path> <output image>" << std::endl;
        return -1;
    }
    
    int w, h, ch;
    
    std::ifstream ifs(argv[1], std::ios::binary);
    ifs.read((char*)&w,  sizeof(int));
    ifs.read((char*)&h,  sizeof(int));
    ifs.read((char*)&ch, sizeof(int));
    
    float* fxy = new float[w*h*2];
    ifs.read((char*)(fxy), w*h*2*sizeof(float));
    ifs.close();
    
    float fmax = 0.0f;
    float fmin = 10000.0f;
    for (int i=0 ; i<w*h ; ++i)
    {
        if (fxy[2*i] < fmin) { fmin = fxy[2*i]; }
        if (fxy[2*i] > fmax) { fmax = fxy[2*i]; }
    }
    std::cout << "F: [" << fmin << ", " << fmax << "]" << std::endl;
    float f = std::log(fmax - fmin + 1.0f);
    
    unsigned char* im = new unsigned char[w*h];
    
    int x2, y2;
    for (int x=0 ; x<w ; ++x)
    {
        for (int y=0 ; y<h ; ++y)
        {
            x2 = (x < (w/2)) ? x + w/2 : x - w/2;
            y2 = (y < (h/2)) ? y + h/2 : y - h/2;
            
            im[y*w + x] = (unsigned char)(std::log(fxy[(y2*w + x2)*2] - fmin + 1.0f) * 255.0f / f);
        }
    }
    stbi_write_png(argv[2], w, h, 1, im, w);
    
    return 1;
}
