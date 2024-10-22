#include <iostream>

#include "raytrace.h"
#include "glhelp.h"
#include "shader.h"

#include "vec3.h"

float vertices[] = {
    // positions         // texture coords
     1.0f,  1.0f, 0.0f,    1.0f, 1.0f, // top right
     1.0f, -1.0f, 0.0f,    1.0f, 0.0f, // bottom right
    -1.0f, -1.0f, 0.0f,    0.0f, 0.0f, // bottom left
    -1.0f,  1.0f, 0.0f,    0.0f, 1.0f  // top left
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};

int main(int argc, char* args[]) {
    // Initialize window

    Camera cam = Camera(1600, 1200, 16.0 / 9.0);

    Context c = Context(cam.screenWidth, cam.screenHeight);
    c.init();

    // VAO/VBO/VEO

    VAO vao = VAO();
    VBOSetup vbos[2];
    vbos[0].ptr    = 0;
    vbos[0].size   = 3;
    vbos[0].type   = GL_FLOAT;
    vbos[0].stride = sizeof(float) * 5;
    vbos[0].offset = (void*)(0);

    vbos[1].ptr    = 1;
    vbos[1].size   = 2;
    vbos[1].type   = GL_FLOAT;
    vbos[1].stride = sizeof(float) * 5;
    vbos[1].offset = (void*)(3*sizeof(float));

    vao.bindVBO(vertices, sizeof(vertices), vbos, sizeof(vbos) / sizeof(VBOSetup));
    vao.bindEBO(indices, sizeof(indices), sizeof(indices) / sizeof(unsigned int));

    // Shaders
    Shader twopassShader = Shader("shaders/texVertex.vs", "shaders/twopassFragment.fs");
    Shader textureShader = Shader("shaders/texVertex.vs", "shaders/texFragment.fs");
    
    twopassShader.use();
    twopassShader.setFloat("width", cam.renderWidth);

    // UBO testing
    /*
    colour checkersa = colour(1.0f, 1.0f, 1.0f);
    colour checkersb = colour(0.3f, 0.4f, 0.5f);

    UBO ubo = UBO((sizeof(checkersa) + sizeof(checkersb)) * 2);
    // Note: This method I still have to worry about padding. Try to recreate the class so I don't.
    ubo.subVec3(checkersa, 0);
    ubo.subVec3(checkersb, 16);

    ubo.bind(0, 0, -1);
    twopassShader.bindUBO(ubo, "checker_colours");
    */

    // Two pass framebuffer testing
    // First pass:
    colour clearColour = colour(0.3f, 0.1f, 0.3f);

    FrameBuffer fb = FrameBuffer(cam.renderWidth, cam.renderHeight);
    fb.bind();
    fb.bindTexture(0);
    twopassShader.use();
    twopassShader.setBool("s", true);

    c.clearBuffer(clearColour);
    vao.draw();

    // Second pass:
    twopassShader.setBool("s", false);

    vao.draw();

    while(!c.isQuit()) {
        c.bind();

        fb.bindTexture(0);
        textureShader.use();

        c.clearBuffer(clearColour);
        vao.draw();

        c.loopEnd();
    }

    return 0;
}