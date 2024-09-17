#include <iostream>
#include <stdlib.h>

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "shader.h"
#include "vec3.h"
#include "sphere.h"
#include "hittable_list.h"

// Start up SDL and create a window
bool init();

// Code for checking attributes I might not need
void check_attributes();

// Event polling code contained in here
void event_handling();

// Free media and shut down SDL
void close();

// SDL Objects
SDL_Window* gWindow = NULL; // Main window
SDL_Surface* gScreenSurface = NULL; // The window's surface
SDL_GLContext gContext = NULL;

// Awful way to handle quitting in event loop (change this)
bool gQuit = false;

const int GLMAJORVERSION = 3;
const int GLMINORVERSION = 3;
const int GLPROFILEMASK = SDL_GL_CONTEXT_PROFILE_CORE;

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

// Screen setup values
auto aspect_ratio = 16.0 / 9.0;
int SCREEN_WIDTH = 1600;
int SCREEN_HEIGHT = 1;

int RENDER_WIDTH = 400;
int RENDER_HEIGHT = 1;

int NUM_SAMPLES = 16;
int BOUNCE_LIMIT = 32;

// Other constants
int MAX_NUM_OBJECTS = 128;
int SPHERE_UBO_SIZE = MAX_NUM_OBJECTS*32;

void check_attributes()
{
    // Check OpenGL attributes
    int check = 0;
    if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &check))
    {
        std::cerr << "SDL could not get major version! SDL Error: " << SDL_GetError() << '\n';
    } else {
        std::cerr << "Requested major version: " << GLMAJORVERSION << ", Actual major version: " << check << '\n';
    }
    if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &check))
    {
        std::cerr << "SDL could not get minor version! SDL Error: " << SDL_GetError() << '\n';
    } else {
        std::cerr << "Requested minor version: " << GLMINORVERSION << ", Actual minor version: " << check << '\n';
    }
    if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &check))
    {
        std::cerr << "SDL could not get profile mask! SDL Error: " << SDL_GetError() << '\n';
    } else {
        std::cerr << "Requested profile mask: " << GLPROFILEMASK << ", Actual profile mask: " << check << '\n';
    }
}

bool init()
{
    // Screen size calculations
    SCREEN_HEIGHT = int(SCREEN_WIDTH / aspect_ratio);
    SCREEN_HEIGHT = (SCREEN_HEIGHT < 1) ? 1 : SCREEN_HEIGHT;

    RENDER_HEIGHT = int(RENDER_WIDTH / aspect_ratio);
    RENDER_HEIGHT = (RENDER_HEIGHT < 1) ? 1 : RENDER_HEIGHT;

    bool success = true;

    // Initialize SDL library (suprised by how much worked without this lol)
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    // Set OpenGL attributes
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if ( SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << '\n';
        success = false;
    } else {
        gWindow = SDL_CreateWindow( "LearnOpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                    SCREEN_WIDTH, SCREEN_HEIGHT, 
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        
        if (gWindow == NULL)
        {
            std::cerr << "SDL Window could not be created! SDL Error: " << SDL_GetError() << '\n';
            success = false;
        } else {
            // Create OpenGL Context for window (and make it current)
            gContext = SDL_GL_CreateContext(gWindow);

            check_attributes();

            // Get Screen Surface from window
            gScreenSurface = SDL_GetWindowSurface(gWindow);

            // Initialize glad using SDLs loader
            // We are casting the SDL_GL_GetProcAddress function object to the GLADloadproc type
            // 
            // NOTE: Without this we would have to define a function pointer prototype for every 
            // OpenGL function we wish to call, then call SDL_GL_GetProcAddress and check that it 
            // returns a valid value.
            if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
            {
                std::cerr << "Failed to initialize GLAD" << std::endl;
                close();
                exit(1);
            }

            // Set OpenGL screen coordinates to match the SDL screen size
            // 
            // NOTE: It is possible to set these values as smaller than the
            // window size. This could be useful if you wanted to put otherwise 
            // rendered ui or other info around the main display.
            glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
    return success;
}

void event_handling()
{
    SDL_Event e;

    // Poll event, removing it from the queue
    while (SDL_PollEvent(&e))
    {
        // If the event is X-ing out of the window set gQuit to true
        if (e.type == SDL_QUIT)
        {
            gQuit = true;
        }

        // Window event handling
        if (e.type == SDL_WINDOWEVENT)
        {
            switch (e.window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    glViewport(0, 0, e.window.data1, e.window.data2);
                    break;
            }
        } 
    }
}

void close()
{
    SDL_DestroyWindow(gWindow); // This also destroys gscreenSurface
    gWindow = NULL;
    gScreenSurface = NULL;

    SDL_Quit();
}


int main(int argc, char* args[])
{
    if(!init())
    {
        exit(1);
    }

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, RENDER_WIDTH, RENDER_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER == GL_FRAMEBUFFER_COMPLETE))
    {
        std::cout << "Frambuffer is complete!!!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Queries??
    /* Get maximum number of vertex attributes we can pass to a vertex shader (it's 16) */
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum Number of Vertex Attributes Supported: " << nrAttributes << std::endl;

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &nrAttributes);
    std::cout << "Maximum Number of Uniform Blocks Supported in Fragment Shader: " << nrAttributes << std::endl;

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &nrAttributes);
    std::cout << "Maximum Uniform Block Size (bytes): " << nrAttributes << std::endl;

    // Create vertex Array object
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // Bind the vertex array
    // Note: This does not appear to do anything, but
    // internally it will save all the vertex attribute
    // settings, configurations and the associated buffer 
    // objects.
    // Note: in practice you would make multiple VAOs for
    // each object/settings configuration, set everything up,
    // then bind and unbind them as you draw each object.
    // Note: Without the array objects, settings would need
    // to be reconfigured before drawing Each Object.
    // Question: If settings are the same for all your objects
    // would you only need to setup the buffers? (I don't think so)
    glBindVertexArray(VAO);
    // Create vertex buffer object
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    // Bind vertex buffer to GL_ARRAY_BUFFER type
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData:
    // Function for passing user-defined data into an opengl buffer
    // GL_ARRAY_BUFFER: specifies we want data from the buffers
    //   currently bound to the GL_ARRAY_BUFFER type
    // GL_STATIC_DRAW: gives Opengl a hint to how the data will be used.
    //   GL_STATIC_DRAW tells Opengl that the data will be static, but drawn
    //   from frequently. This means it should be optimized for fast reads,
    //   but writes are not as important
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // We need to tell OpenGL how it should interpret the
    // vertex data:
    // (Note: This is because you can specify your own input
    //  as a vertex attribute)
    // Lots of arguments:
    // 1 - vertex attribute number (specified in vertex shader)
    // 2 - size of vertex attribute (how many vertices)
    // 3 - data type
    // 4 - Normalize data? (?)
    // 5 - Stride - space between vertex attributes
    // 6 - Offset in buffer
    // Note: This will be called on the VBO currently bound to
    //   GL_ARRAY_BUFFER (unchanged in this case)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    // Finally Enable the vertex attribute 
    // (using its location as an arg)
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // SHADER CREATION:
    Shader ourShader("shaders/testVertex.vs", "shaders/testFragment.fs");
    Shader texShader("shaders/texVertex.vs", "shaders/texFragment.fs");

    // Raytracing setup

    auto focal_length = 1.0;
    auto viewport_height = 2.0; // (arbitrary)
    auto viewport_width = (double(RENDER_WIDTH) / double(RENDER_HEIGHT)) * viewport_height;

    vec3 camera_origin = vec3(0.0f, 0.0f, 0.0f);
    vec3 viewport_top_left = camera_origin - vec3(0, 0, focal_length)
                                            - vec3(viewport_width/2, 0, 0)
                                             - vec3(0, -viewport_height/2, 0);

    vec3 delta_u = vec3(viewport_width / RENDER_WIDTH, 0.0f, 0.0f);
    vec3 delta_v = vec3(0.0f, -viewport_height / RENDER_HEIGHT, 0.0f);

    std::cout << "RENDER_HEIGHT: " << RENDER_HEIGHT << std::endl;
    std::cout << "RENDER_WIDTH: " << RENDER_WIDTH << std::endl;

    std::cout << "viewport_height: " << viewport_height << std::endl;
    std::cout << "viewport_width: " << viewport_width << std::endl;
    std::cout << "viewport_top_left: " << viewport_top_left << std::endl;
    std::cout << "delta_u: " << delta_u << std::endl;
    std::cout << "delta_v: " << delta_v << std::endl;

    // Set up UBO data

    unsigned int uboBlock;
    glGenBuffers(1, &uboBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
    glBufferData(GL_UNIFORM_BUFFER, 4096, NULL, GL_STATIC_DRAW); // Allocate 4096 bytes for UBO
    // Note, can hold 128 32byte spheres
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind UBO in sphere to uboBlock
    unsigned int uniformBlockIndexSphere = glGetUniformBlockIndex(ourShader.ID, "Spheres");
    glUniformBlockBinding(ourShader.ID, uniformBlockIndexSphere, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboBlock, 0, 4096);

    // Add spheres
    hittable_list objects = hittable_list();

    sphere sphere1 = sphere(0.5f, vec3(0.0f, 0.0f, -1.0f), 0);
    sphere sphere2 = sphere(100.0f, vec3(0.0f, -100.5f, -1.0f), 0);
    sphere sphere3 = sphere(0.125f, vec3(-0.5f, 0.0f, -0.5f), 0);
    sphere sphere4 = sphere(0.125f, vec3(0.5f, 0.0f, -0.5f), 0);
    sphere sphere5 = sphere(500.0f, vec3(0.0f, 0.0f, 0.0f), 0);

    //objects.add(uboBlock, sphere5);
    //objects.add(uboBlock, sphere3);
    objects.add(uboBlock, sphere1);
    objects.add(uboBlock, sphere2);
    //objects.add(uboBlock, sphere4);

    // First pass render to FBO texture

    // bind frame buffer for offscreen rendering
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);

    // Clear colour buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate shader
    ourShader.use();

    // Shader uniforms
    uint32_t timeValue = SDL_GetTicks();
    ourShader.setUint("time_u32t", timeValue);

    ourShader.setInt("num_samples", NUM_SAMPLES);
    ourShader.setInt("bounce_limit", BOUNCE_LIMIT);

    ourShader.setVec3("delta_u", delta_u);
    ourShader.setVec3("delta_v", delta_v);
    ourShader.setVec3("camera_origin", camera_origin);
    ourShader.setVec3("viewport_top_left", viewport_top_left);
    ourShader.setInt("num_spheres", objects.num);

    // Draw triangles
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    while (!gQuit)
    {
        // Input

        // Rendering

        // bind back to default frame buffer to display rendered texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        texShader.use();
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        // Events
        event_handling();

        // Swap buffers
        SDL_GL_SwapWindow(gWindow);
    }
    
    close();

    return 0;
}