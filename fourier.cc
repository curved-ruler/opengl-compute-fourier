
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>


#define GL_CHECK_ERROR() check_error(__FILE__, __LINE__)
GLenum check_error(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               error = "Undefined error";
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}


GLFWwindow* window;
GLuint programx;
GLuint programy;


const char* shaderx = R"compsx(
    
#version 460

layout(local_size_x = 1, local_size_y = 1) in;

layout(std430, binding=0) buffer buf0 { vec2 imin[]; };
layout(std430, binding=1) buffer buf1 { vec2 fxout[]; };
uniform int w;
uniform int h;

const float pi = 3.14159265358;

// complex multiplication
vec2 cmul (in vec2 z1, in vec2 z2)
{
    return vec2(z1.x*z2.x - z1.y*z2.y, z1.x*z2.y + z1.y*z2.x);
}

// 1D DFT for row
void main ()
{
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);
    vec2 res = vec2(0.0, 0.0);
    
    for (int n=0 ; n<w ; ++n)
    {
        float c  = float(n) * float(x) * -2.0 * pi / float(w);
        vec2  ec = vec2(cos(c), sin(c)); // exp(ic)
        
        vec2 s = imin[y*w + n];
        res += cmul(s, ec) / float(w);
        
        //uvec4 s = texture(texsampler, vec2((float(n)+0.5)/float(w), (float(y)+0.5)/float(h)));
        //res += cmul(vec2(float(s.x)/255.0, 0.0), ec) / float(w);
    }
    
    fxout[y*w + x] = res;
}

)compsx";

const char* shadery = R"compsy(
    
#version 460

layout(local_size_x = 1, local_size_y = 1) in;

layout(std430, binding=0) buffer buf0 { vec2 fxin[]; };
layout(std430, binding=1) buffer buf1 { vec2 fout[]; };
uniform int w;
uniform int h;

const float pi = 3.14159265358;

// complex multiplication
vec2 cmul (in vec2 z1, in vec2 z2)
{
    return vec2(z1.x*z2.x - z1.y*z2.y, z1.x*z2.y + z1.y*z2.x);
}

// 1D DFT for column
void main ()
{
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);
    vec2 res = vec2(0.0, 0.0);
    
    for (int n=0 ; n<h ; ++n)
    {
    
        float c  = float(n) * float(y) * -2.0 * pi / float(h);
        vec2  ec = vec2(cos(c), sin(c)); // exp(ic)
        
        vec2 s = fxin[n*w + x];
        
        res += cmul(s, ec) / float(h);
    }
    fout[y*w + x] = res;
}

)compsy";




int compile_glprog (GLuint& program, const char* shd)
{
    GLint   success = 0;
    GLint   length = 0;
    GLuint  compshader = glCreateShader(GL_COMPUTE_SHADER);
    program = glCreateProgram();
    GLchar* log = 0;
    
    glShaderSource(compshader, 1, &shd, NULL);
    glCompileShader(compshader);
    
    glGetShaderiv(compshader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderiv(compshader, GL_INFO_LOG_LENGTH, &length);
        log = new GLchar[length+2];
        GLint x;
        glGetShaderInfoLog(compshader, length+1, &x, log);
        
        std::cout << "Shader ERROR:" << std::endl << log << std::endl;
        int line = 1;
        std::cout << std::setw(2) << line << ": ";
        std::string shdstr = shd;
        for (size_t i=0 ; i<shdstr.size() ; ++i)
        {
            if (shdstr[i] == '\n') { ++line; std::cout << std::endl << std::setw(2) << line << ": "; }
            else { std::cout << shdstr[i]; }
        }
        std::cout << std::endl;
        delete[] log;
        return -1;
    }
    
    glAttachShader(program, compshader);

    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        log = new GLchar[length+2];
        GLint x;
        glGetProgramInfoLog(program, length+1, &x, log);
        
        std::cout << "Shader link ERROR:" << std::endl << log << std::endl;
        delete[] log;
        return -1;
    }
    
    return 1;
}

int main (int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: fourier <input image path> <out data file>" << std::endl;
        return -1;
    }
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(640, 480, "", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -2;
    }
    
    int ret = compile_glprog(programx, shaderx);
    if (ret < 0) return ret;
    ret = compile_glprog(programy, shadery);
    
    
    int w, h, ch;
    unsigned char* im = stbi_load(argv[1], &w, &h, &ch, 0);
    if (stbi_failure_reason())
    {
        std::cout << "Stbi ERROR: " << stbi_failure_reason() << std::endl;
        return -3;
    }
    std::cout << "Reading image " << argv[1] << ", w-h-ch: " << w << " - " << h << " - " << ch <<std::endl;
    if (ch > 1)
    {
        std::cout << "Currently handling only single-channel images" << std::endl;
        delete[] im;
        return -4;
    }
    
    float* fxy = new float[w*h*2];
    for (int i=0 ; i<w*h ; ++i) { fxy[2*i] = im[i]; fxy[2*i+1] = 0.0f; }
    
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    GLuint fxbuf = 0;
    glGenBuffers(1, &fxbuf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fxbuf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, w*h*2*sizeof(float), 0, GL_DYNAMIC_COPY);
    
    GLuint fxybuf = 0;
    glGenBuffers(1, &fxybuf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fxybuf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, w*h*2*sizeof(float), fxy, GL_DYNAMIC_COPY);
    
    
    // *********************** 1st pass ***********************
    glUseProgram(programx);
    glUniform1i(glGetUniformLocation(programx, "w"), w);
    glUniform1i(glGetUniformLocation(programx, "h"), h);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fxybuf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, fxbuf);
    
    glDispatchCompute(w, h, 1);
    
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    
    
    // *********************** 2nd pass ***********************
    glUseProgram(programy);
    glUniform1i(glGetUniformLocation(programy, "w"), w);
    glUniform1i(glGetUniformLocation(programy, "h"), h);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fxbuf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, fxybuf);
    
    glDispatchCompute(w, h, 1);
    
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    
    
    // ******************** data retrieval ********************
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fxybuf);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, w*h*2*sizeof(float), fxy);
    
    GL_CHECK_ERROR();
    
    std::ofstream ofs(argv[2], std::ios::binary);
    ofs.write((const char*)(&w),  sizeof(int));
    ofs.write((const char*)(&h),  sizeof(int));
    ofs.write((const char*)(&ch), sizeof(int));
    ofs.write((const char*)(fxy), w*h*2*sizeof(float));
    //ofs.write((const char*)(im), w*h*sizeof(unsigned char));
    ofs.close();
    
    
    delete[] im;
    delete[] fxy;
    glDeleteBuffers(1, &fxbuf);
    glDeleteBuffers(1, &fxybuf);
    glDeleteVertexArrays(1, &VAO);
    
    glfwTerminate();
    
    return 1;
}
