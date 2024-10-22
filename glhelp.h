#ifndef GLHELP_H
#define GLHELP_H

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include <iostream>
#include <string>

// Main window management/setup (context?)

class Context {
    private:
    SDL_Window* glWindow = NULL; // Main window
    SDL_Surface* glScreenSurface = NULL; // The window's surface
    SDL_GLContext glContext = NULL;

    const unsigned int SDL_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    const int glMajorVersion = 3;
    const int glMinorVersion = 3;
    const int glProfileMask = SDL_GL_CONTEXT_PROFILE_CORE;

    int screenWidth = 1;
    int screenHeight = 1;

    bool quit = false;

    public:

    Context(int myScreenWidth, int myScreenHeight) : screenWidth{myScreenWidth}, screenHeight{myScreenHeight} {};

    void check_attributes()
    {
        int check = 0;
        if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &check))
        {
            std::cerr << "SDL could not get major version! SDL Error: " << SDL_GetError() << '\n';
        } else {
            std::cerr << "Requested major version: " << glMajorVersion << ", Actual major version: " << check << '\n';
        }
        if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &check))
        {
            std::cerr << "SDL could not get minor version! SDL Error: " << SDL_GetError() << '\n';
        } else {
            std::cerr << "Requested minor version: " << glMinorVersion << ", Actual minor version: " << check << '\n';
        }
        if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &check))
        {
            std::cerr << "SDL could not get profile mask! SDL Error: " << SDL_GetError() << '\n';
        } else {
            std::cerr << "Requested profile mask: " << glProfileMask << ", Actual profile mask: " << check << '\n';
        }
    }

    bool init() 
    {
        bool success = true;

        SDL_Init(SDL_flags);

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion);
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion);
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        if ( SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << '\n';
            success = false;
        } else {
            glWindow = SDL_CreateWindow( "LearnOpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                        screenWidth, screenHeight, 
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            
            if (glWindow == NULL)
            {
                std::cerr << "SDL Window could not be created! SDL Error: " << SDL_GetError() << '\n';
                success = false;
            } else {
                // Create OpenGL Context for window (and make it current)
                glContext = SDL_GL_CreateContext(glWindow);

                check_attributes();

                // Get Screen Surface from window
                glScreenSurface = SDL_GetWindowSurface(glWindow);

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
                glViewport(0, 0, screenWidth, screenHeight);
            }
        }

        return success;
    }

    void close()
    {
        SDL_DestroyWindow(glWindow); // This also destroys gscreenSurface
        glWindow = NULL;
        glScreenSurface = NULL;

        SDL_Quit();
    }

    // Event handling

    void event_handling()
    {
        SDL_Event e;

        // Poll event, removing it from the queue
        while (SDL_PollEvent(&e))
        {
            // If the event is X-ing out of the window set gQuit to true
            if (e.type == SDL_QUIT)
            {
                quit = true;
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

    void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
    }

    bool isQuit() { return quit; }

    void clearBuffer(colour c) {
        glClearColor(c[0], c[1], c[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void loopEnd() {
        SDL_GL_SwapWindow(glWindow);
    }
};

// Other buffer management

// Vertex buffers

struct VBOSetup {
    // Just a way to store the arguments for glVertexAttribPointer
    unsigned int ptr;
    unsigned int size;
    int type;
    unsigned int stride;
    void* offset;
};

class VAO {
    private:
    bool hasEBO = false;
    int EBOSize = 0;

    public:
    unsigned int id;

    VAO() { glGenVertexArrays(1, &id); };

    /* Note: Decided to pass vertex attrib pointers settings as an array, as there can
    be multiple per VBO? This is obviously limiting, as I don't have a nice way of setting
    more later, but if the data is static then why would I (This will probably turn out to be
    a bad idea later)*/
    void bindVBO(void *data, unsigned int data_size, VBOSetup *vbos, int num_bindings) {
        glBindVertexArray(id);

        unsigned int vbo;
        glGenBuffers(1, &vbo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);

        VBOSetup ref;

        for (int i=0; i<num_bindings; i++) {
            ref = vbos[i];

            glVertexAttribPointer(ref.ptr, ref.size, ref.type, GL_FALSE, ref.stride, ref.offset);
            // Note: I tried to look up why this call is separate and it seems there Is a reason, but
            // it will only happen if I'm not sourcing my vertex data from an array, which I will be
            // doing. So this is fine I think.
            glEnableVertexAttribArray(ref.ptr);
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void bindEBO(void *data, unsigned int data_size, int n) {
        glBindVertexArray(id);

        unsigned int ebo;
        glGenBuffers(1, &ebo);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        hasEBO = true;
        EBOSize = n;
    }

    void draw() {
        if (!hasEBO) {
            // Currently just forcing use of EBO bc it makes life easier
            return;
        }

        glBindVertexArray(id);
        glDrawElements(GL_TRIANGLES, EBOSize, GL_UNSIGNED_INT, 0);
    }
};

// Framebuffers

class FrameBuffer {
    public:
    unsigned int id = 0;
    unsigned int rbo = 0;
    unsigned int tex = 0;

    int width = 0;
    int height = 0;

    public:
    FrameBuffer(int myWidth, int myHeight) : width{myWidth}, height{myHeight} {
        // Create Framebuffer
        glGenFramebuffers(1, &id);
        glBindFramebuffer(GL_FRAMEBUFFER, id);

        // Create Texture
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Attach texture to framebuffer
        // Note: last argument is mipmap level (keep 0)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        // Create Renderbuffer
        // Note: (Still not sure why this is required if I'm only using the texture)
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo); 
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);  
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Attach renderbuffer to framebuffer
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "Framebuffer is incomplete!!!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~FrameBuffer() {
        remove();
    }

    void bind() { 
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        // Note: Also binding viewport here bc why not
        // This will be annoying if I find another reason to bind
        // the framebuffer after creation, but currently I have none.
        glViewport(0, 0, width, height); 
    }

    void bindTexture(unsigned int textureUnit) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, tex);
        glActiveTexture(GL_TEXTURE0);
    }

    private:
    void remove() { glDeleteFramebuffers(1, &id); }
};

// UBO management
class UBO {
    public:
    unsigned int id;
    unsigned int size;
    unsigned int binding_point;

    UBO(unsigned int mySize) : size{mySize}
    {
    glGenBuffers(1, &id);
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    /* Notes:
    - The block index is the index of the named block within the shader
    - The binding point is abstract and unrelated to the shader, it just allows multiple 
      shaders to bind to the same binding point (note, this operation is unrelated to the
      UBO until glBindBufferRange is called to bind it to that binding point.)
    - I'll bind the block index of the shader to the binding points in the shader helper code. */
    void bind(unsigned int my_binding_point, int min, int max)
    {
    if (max == -1) {
        min = 0;
        max = size;
    }
    binding_point = my_binding_point;
    glBindBufferRange(GL_UNIFORM_BUFFER, binding_point, id, min, max);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void sub(void *data, unsigned int size, int start) 
    {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, start, size, &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void subVec3(vec3 &data, int start)
    {
    float vec4[4];
    vec4[0] = data[0];
    vec4[1] = data[1];
    vec4[2] = data[2];
    vec4[3] = 0.0f;

    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, start, sizeof(float) * 4, &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};

#endif