
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
    
    int loglevel = 1;
    int w, h, ch;
    
    std::ifstream ifs(argv[1], std::ios::binary);
    ifs.read((char*)&w,  sizeof(int));
    ifs.read((char*)&h,  sizeof(int));
    ifs.read((char*)&ch, sizeof(int));
    
    float* fxy = new float[w*h*2];
    ifs.read((char*)(fxy), w*h*2*sizeof(float));
    ifs.close();
    
    float fmax = 0.0f;
    for (int i=0 ; i<w*h ; ++i)
    {
        float f = std::sqrt( fxy[2*i]*fxy[2*i] + fxy[2*i+1]*fxy[2*i+1] );
        if (f > fmax) { fmax = f; }
    }
    
    float division = fmax * 255.0f;
    for (int l=0 ; l<loglevel ; ++l)
    {
        division = std::log(division + 1.0f);
    }
    division /= 10.0f;
    std::cout << "F: " << fmax << ", D: " << division << std::endl;
    
    unsigned char* im = new unsigned char[w*h];
    
    float fm2 = 0.0f;
    int x2, y2, i;
    for (int x=0 ; x<w ; ++x)
    {
        for (int y=0 ; y<h ; ++y)
        {
            x2 = (x < (w/2)) ? x + w/2 : x - w/2;
            y2 = (y < (h/2)) ? y + h/2 : y - h/2;
            i  = y2*w + x2;
            
            float f = std::sqrt( fxy[2*i]*fxy[2*i] + fxy[2*i+1]*fxy[2*i+1] );
            for (int l=0 ; l<loglevel ; ++l) { f = std::log(f + 1.0f); }
            f /= division;
            if (f > fm2)  fm2 = f;
            if (f > 1.0f) f = 1.0f;
            
            im[y*w + x] = (unsigned char)(f*255.0f);
        }
    }
    std::cout << "F2: " << fm2 << std::endl;
    stbi_write_png(argv[2], w, h, 1, im, w);
    
    return 1;
}
