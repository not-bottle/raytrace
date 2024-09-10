#include <iostream>
#include <stdlib.h>

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "shader.h"
#include "vec3.h"

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
    // first triangle 
    -1.0f,  1.0f, 0.0f, // top left
     1.0f,  1.0f, 0.0f, // top right
     1.0f, -1.0f, 0.0f, // bottom right
    // second triangle
     1.0f, -1.0f, 0.0f, // bottom right
    -1.0f, -1.0f, 0.0f, // bottom left
    -1.0f,  1.0f, 0.0f, // top left
};

// Screen setup values
auto aspect_ratio = 16.0 / 9.0;
int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 1;

// Other constants
int MAX_NUM_OBJECTS = 64;

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

    // SHADER CREATION:
    Shader ourShader("shaders/testVertex.vs", "shaders/testFragment.fs");

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // Finally Enable the vertex attribute 
    // (using its location as an arg)
    glEnableVertexAttribArray(0);

    // Raytracing setup

    auto focal_length = 1.0;
    auto viewport_height = 2.0; // (arbitrary)
    auto viewport_width = (double(SCREEN_WIDTH) / double(SCREEN_HEIGHT)) * viewport_height;

    vec3 camera_origin = vec3(0.0f, 0.0f, 0.0f);
    vec3 viewport_top_left = camera_origin - vec3(0, 0, focal_length)
                                            - vec3(viewport_width/2, 0, 0)
                                             - vec3(0, -viewport_height/2, 0);

    vec3 delta_u = vec3(viewport_width / SCREEN_WIDTH, 0.0f, 0.0f);
    vec3 delta_v = vec3(0.0f, -viewport_height / SCREEN_HEIGHT, 0.0f);

    std::cout << "SCREEN_HEIGHT: " << SCREEN_HEIGHT << std::endl;
    std::cout << "SCREEN_WIDTH: " << SCREEN_WIDTH << std::endl;

    std::cout << "viewport_height: " << viewport_height << std::endl;
    std::cout << "viewport_width: " << viewport_width << std::endl;
    std::cout << "viewport_top_left: " << viewport_top_left << std::endl;
    std::cout << "delta_u: " << delta_u << std::endl;
    std::cout << "delta_v: " << delta_v << std::endl;

    // Set up UBO data

    unsigned int uboBlock;
    glGenBuffers(1, &uboBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
    glBufferData(GL_UNIFORM_BUFFER, 32, NULL, GL_STATIC_DRAW); // Allocate 16 bytes for UBO
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind UBO in sphere to uboBlock
    unsigned int uniformBlockIndexSphere = glGetUniformBlockIndex(ourShader.ID, "Sphere");
    glUniformBlockBinding(ourShader.ID, uniformBlockIndexSphere, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboBlock, 0, 32);

    // Add radius
    float sphere_radius = 0.5f;
    glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float), &sphere_radius);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Add origin
    vec3 sphere_origin = vec3(0.5, 0.7, 1);
    float origin2[3] = {0.0f, 0.0f, -1.0f};
    glBindBuffer(GL_UNIFORM_BUFFER, uboBlock);
    glBufferSubData(GL_UNIFORM_BUFFER, 16, sizeof(float) * 3, origin2);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    while (!gQuit)
    {
        // Input

        // Rendering
        // Clear colour buffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Activate shader
        ourShader.use();

        // Shader uniforms
        ourShader.setVec3("delta_u", delta_u);
        ourShader.setVec3("delta_v", delta_v);
        ourShader.setVec3("camera_origin", camera_origin);
        ourShader.setVec3("viewport_top_left", viewport_top_left);

        // Draw triangles
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Events
        event_handling();

        // Swap buffers
        SDL_GL_SwapWindow(gWindow);
    }
    
    close();

    return 0;
}