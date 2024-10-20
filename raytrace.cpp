#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <algorithm>

#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "raytrace.h"
#include "glhelp.h"

#include "randgen.h"

#include "shader.h"
#include "vec3.h"
#include "sphere.h"
#include "hittable_list.h"

#include "material_list.h"
#include "material.h"

void load_three_spheres(material_list materials, hittable_list objects, unsigned int matUBO, unsigned int sphereUBO);
void load_final_scene(material_list materials, hittable_list objects, unsigned int matUBO, unsigned int sphereUBO);

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

int NUM_SAMPLES = 512;
uint32_t BOUNCE_LIMIT = 50;

// Other constants
const int MAX_NUM_OBJECTS = 128;
const int SPHERE_UBO_SIZE = MAX_NUM_OBJECTS*32;

int RANDOM_ARRAY_SIZE = 2048;


int main(int argc, char* args[])
{
    int screen_width = 1600;
    int render_width = 1200;
    float aspect_ratio = 16.0/9.0;
        
    windowHelp windowhelp = windowHelp(screen_width, render_width, aspect_ratio);

    glHelp glhelp = glHelp(windowhelp.SCREEN_WIDTH, windowhelp.SCREEN_HEIGHT);

    if(!glhelp.init())
    {
        exit(1);
    }

    fb_help perlinfb, upscalefb;

    createFrameBuffer(perlinfb);
    createFrameBuffer(upscalefb);

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
    Shader noiseShader("shaders/testVertex.vs", "shaders/prng.fs");
    Shader texShader("shaders/texVertex.vs", "shaders/texFragment.fs");

    glBindFramebuffer(GL_FRAMEBUFFER, perlinfb.fbo);
    glViewport(0, 0, windowhelp.RENDER_WIDTH, windowhelp.RENDER_HEIGHT);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    noiseShader.use();

    noiseShader.setUint("timevalue", 3568u);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Raytracing setup

    orientation uvw;

    uvw.lookfrom = point3(13, 2, 3);
    uvw.lookat = point3(0, 0, 0);
    uvw.vup = vec3(0, 1, 0);
    uvw.vfov = 20.0;
    uvw.defocus_angle = 0.6;
    uvw.focus_dist = 10;

    cameraSetup(uvw, windowhelp.RENDER_WIDTH, windowhelp.RENDER_HEIGHT);

    // Set up UBO data

    // MATERIALS

    unsigned int matUBO;
    glGenBuffers(1, &matUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matUBO);
    glBufferData(GL_UNIFORM_BUFFER, 20000, NULL, GL_STATIC_DRAW); // Allocate 4096 bytes for UBO
    // Note, can hold 128 32byte materials
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind UBO in sphere to uboBlock
    unsigned int uniformBlockIndexMat = glGetUniformBlockIndex(ourShader.ID, "Materials");
    glUniformBlockBinding(ourShader.ID, uniformBlockIndexMat, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, matUBO, 0, 20000);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // SPHERES

    unsigned int sphereUBO;
    glGenBuffers(1, &sphereUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, sphereUBO);
    glBufferData(GL_UNIFORM_BUFFER, 20000, NULL, GL_STATIC_DRAW); // Allocate 4096 bytes for UBO
    // Note, can hold 128 32byte spheres
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind UBO in sphere to uboBlock
    unsigned int uniformBlockIndexSphere = glGetUniformBlockIndex(ourShader.ID, "Spheres");
    glUniformBlockBinding(ourShader.ID, uniformBlockIndexSphere, 1);
    glBindBufferRange(GL_UNIFORM_BUFFER, 1, sphereUBO, 0, 20000);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // RANDOM GENERATION
    randgen r {};
    vec4 rand_unit_vectors[RANDOM_ARRAY_SIZE];
    vec4 rand_unit_disks[RANDOM_ARRAY_SIZE];
    vec4 rand_squares[RANDOM_ARRAY_SIZE];
    vec4 rand_vectors[RANDOM_ARRAY_SIZE];

    r.gen_unit_vectors(rand_unit_vectors, RANDOM_ARRAY_SIZE);
    r.gen_unit_disks(rand_unit_disks, RANDOM_ARRAY_SIZE);
    r.gen_rand_squares(rand_squares, RANDOM_ARRAY_SIZE);
    r.gen_rand_vectors(rand_vectors, RANDOM_ARRAY_SIZE);

    unsigned int randUBO_1, randUBO_2;

    glGenBuffers(1, &randUBO_1);
    glBindBuffer(GL_UNIFORM_BUFFER, randUBO_1);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(rand_unit_vectors) + sizeof(rand_unit_disks), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    unsigned int uniformBlockIndexRand_1 = glGetUniformBlockIndex(ourShader.ID, "Precomp_1");
    glUniformBlockBinding(ourShader.ID, uniformBlockIndexRand_1, 3);
    glBindBufferRange(GL_UNIFORM_BUFFER, 3, randUBO_1, 0, sizeof(rand_unit_vectors) + sizeof(rand_unit_disks));
    glBindBuffer(GL_UNIFORM_BUFFER, randUBO_1);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(rand_unit_vectors), &rand_unit_vectors);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, randUBO_1);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(rand_unit_vectors), sizeof(rand_unit_disks), &rand_unit_disks);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &randUBO_2);
    glBindBuffer(GL_UNIFORM_BUFFER, randUBO_2);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(rand_unit_disks) + sizeof(rand_vectors), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    unsigned int uniformBlockIndexRand_2 = glGetUniformBlockIndex(ourShader.ID, "Precomp_2");
    glUniformBlockBinding(ourShader.ID, uniformBlockIndexRand_2, 4);
    glBindBufferRange(GL_UNIFORM_BUFFER, 4, randUBO_2, 0, sizeof(rand_unit_disks) + sizeof(rand_vectors));
    glBindBuffer(GL_UNIFORM_BUFFER, randUBO_2);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(rand_squares), &rand_squares);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, randUBO_2);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(rand_squares), sizeof(rand_vectors), &rand_vectors);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Set up scene

    material_list materials = material_list();
    hittable_list objects = hittable_list();

    lambertian mat_ground = lambertian(vec3(0.2, 0.1, 0.0));
    // lambertian mat_centre = lambertian(vec3(0.1, 0.2, 0.5));
    // dialectric mat_left = dialectric(1.5);
    // dialectric mat_left_bubble = dialectric(1.0/1.5);
    // metallic mat_right = metallic(vec3(0.8, 0.6, 0.2), 0.0);

    // lambertian mat_left2 = lambertian(vec3(1.0, 0.0, 0.0));
    // lambertian mat_right2 = lambertian(vec3(0.0, 0.0, 1.0));

    // materials.add(matUBO, mat_ground);
    // materials.add(matUBO, mat_centre);
    // materials.add(matUBO, mat_left);
    // materials.add(matUBO, mat_right);
    // materials.add(matUBO, mat_left_bubble);

    // materials.add(matUBO, mat_left2);
    // materials.add(matUBO, mat_right2);

    // Add spheres

    // sphere ground = sphere(1000.0, vec3(0.0, -1000.5, -1), &mat_ground);
    // sphere centre = sphere(0.5, vec3(0.0, 0.0, -1.2), &mat_centre);
    // sphere left = sphere(0.5, vec3(-1.0, 0.0, -1.0), &mat_left);
    // sphere left_bubble = sphere(0.4, vec3(-1.0, 0.0, -1.0), &mat_left_bubble);
    // sphere right = sphere(0.5, vec3(1.0, 0.0, -1.0), &mat_right);

    // objects.add(sphereUBO, ground);
    // objects.add(sphereUBO, centre);
    // objects.add(sphereUBO, left);
    // objects.add(sphereUBO, right);

    lambertian ground_material = lambertian(colour(0.5, 0.5, 0.5));
    sphere ground = sphere(1000, point3(0, -1000, 0), &ground_material);
    materials.add(matUBO, ground_material);
    objects.add(sphereUBO, ground);

    dialectric material1 = dialectric(1.5);
    lambertian material2 = lambertian(colour(0.4, 0.2, 0.1));
    metallic   material3 = metallic(colour(0.7, 0.6, 0.5), 0.0);

    materials.add(matUBO, material1);
    materials.add(matUBO, material2);
    materials.add(matUBO, material3);

    sphere     sphere1 = sphere(1.0, point3(0, 1, 0), &material1);
    sphere     sphere2 = sphere(1.0, point3(-4, 1, 0), &material2);
    sphere     sphere3 = sphere(1.0, point3(4, 1, 0), &material3);

    objects.add(sphereUBO, sphere1);
    objects.add(sphereUBO, sphere2);
    objects.add(sphereUBO, sphere3);

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_float();
            point3 centre(a + 0.9*random_float(), 0.2, b + 0.9*random_float());

            if ((centre - point3(4, 0.2, 0)).length() > 0.9) {

                if (choose_mat < 0.5) {
                    auto albedo = colour::random() * colour::random();
                    lambertian sphere_material = lambertian(albedo);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                } else if (choose_mat < 0.8) {
                    auto albedo = colour::random(0.5, 1);
                    auto fuzz = random_float(0, 0.5);
                    metallic sphere_material = metallic(albedo, fuzz);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                } else {
                    dialectric sphere_material = dialectric(1.5);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                }
            }
        }
    }

    // First pass render to FBO texture

    // bind frame buffer for offscreen rendering
    glBindFramebuffer(GL_FRAMEBUFFER, upscalefb.fbo);
    glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
    glBindTexture(GL_TEXTURE_2D, perlinfb.tex);

    // Clear colour buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate shader
    ourShader.use();

    float chunk_size = 50.0f;
    float x_passes = RENDER_HEIGHT / chunk_size;
    float y_passes = RENDER_WIDTH / chunk_size;

    float xmin, xmax, ymin, ymax;

    xmin = 0.0f;
    xmax = xmin + chunk_size;

    // Shader uniforms
    ourShader.setFloat("X_MIN", xmin);
    ourShader.setFloat("X_MAX", xmax);
    ourShader.setFloat("Y_MIN", ymin);
    ourShader.setFloat("Y_MAX", ymax);

    uint32_t timeValue = SDL_GetTicks();
    ourShader.setUint("time_u32t", timeValue);

    ourShader.setInt("num_samples", NUM_SAMPLES);
    ourShader.setUint("bounce_limit", BOUNCE_LIMIT);

    ourShader.setVec3("delta_u", uvw.delta_u);
    ourShader.setVec3("delta_v", uvw.delta_v);
    ourShader.setVec3("camera_origin", uvw.lookfrom);
    ourShader.setVec3("viewport_top_left", uvw.viewport_top_left);

    ourShader.setFloat("defocus_angle", uvw.defocus_angle);
    ourShader.setVec3("defocus_disk_u", uvw.defocus_disk_u);
    ourShader.setVec3("defocus_disk_v", uvw.defocus_disk_v);

    ourShader.setInt("num_spheres", objects.num);

    // Draw triangles
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    while (!glhelp.quit)
    {
        // Input

        // Rendering

        // bind back to default frame buffer to display rendered texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        
        texShader.use();
        glBindTexture(GL_TEXTURE_2D, upscalefb.tex);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glhelp.loopEnd();
    }
    
    glhelp.close();

    return 0;
}

void load_three_spheres(material_list materials, hittable_list objects, unsigned int matUBO, unsigned int sphereUBO) {
    // Add Materials

    lambertian mat_ground = lambertian(vec3(0.8, 0.8, 0.0));
    lambertian mat_centre = lambertian(vec3(0.1, 0.2, 0.5));
    dialectric mat_left = dialectric(1.5);
    dialectric mat_left_bubble = dialectric(1.0/1.5);
    metallic mat_right = metallic(vec3(0.8, 0.6, 0.2), 0.0);

    lambertian mat_left2 = lambertian(vec3(1.0, 0.0, 0.0));
    lambertian mat_right2 = lambertian(vec3(0.0, 0.0, 1.0));

    materials.add(matUBO, mat_ground);
    materials.add(matUBO, mat_centre);
    materials.add(matUBO, mat_left);
    materials.add(matUBO, mat_right);
    materials.add(matUBO, mat_left_bubble);

    materials.add(matUBO, mat_left2);
    materials.add(matUBO, mat_right2);

    // Add spheres

    sphere ground = sphere(100.0, vec3(0.0, -100.5, -1.0), &mat_ground);
    sphere centre = sphere(0.5, vec3(0.0, 0.0, -1.2), &mat_centre);
    sphere left = sphere(0.5, vec3(-1.0, 0.0, -1.0), &mat_left);
    sphere left_bubble = sphere(0.4, vec3(-1.0, 0.0, -1.0), &mat_left_bubble);
    sphere right = sphere(0.5, vec3(1.0, 0.0, -1.0), &mat_right);

    objects.add(sphereUBO, ground);
    objects.add(sphereUBO, centre);
    objects.add(sphereUBO, left);
    objects.add(sphereUBO, right);
}

void load_final_scene(material_list materials, hittable_list objects, unsigned int matUBO, unsigned int sphereUBO) {
    lambertian ground_material = lambertian(colour(0.5, 0.5, 0.5));
    sphere ground = sphere(1000, point3(0, -1000, 0), &ground_material);
    materials.add(matUBO, ground_material);
    objects.add(sphereUBO, ground);

    dialectric material1 = dialectric(1.5);
    lambertian material2 = lambertian(colour(0.4, 0.2, 0.1));
    metallic   material3 = metallic(colour(0.7, 0.6, 0.5), 0.0);

    materials.add(matUBO, material1);
    materials.add(matUBO, material2);
    materials.add(matUBO, material3);

    sphere     sphere1 = sphere(1.0, point3(0, 1, 0), &material1);
    sphere     sphere2 = sphere(1.0, point3(-4, 1, 0), &material2);
    sphere     sphere3 = sphere(1.0, point3(4, 1, 0), &material3);

    objects.add(sphereUBO, sphere1);
    objects.add(sphereUBO, sphere2);
    objects.add(sphereUBO, sphere3);

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_float();
            point3 centre(a + 0.9*random_float(), 0.2, b + 0.9*random_float());

            if ((centre - point3(4, 0.2, 0)).length() > 0.9) {

                if (choose_mat < 0.2) {
                    auto albedo = colour::random() * colour::random();
                    lambertian sphere_material = lambertian(albedo);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                } else if (choose_mat < 0.4) {
                    auto albedo = colour::random(0.5, 1);
                    auto fuzz = random_float(0, 0.5);
                    metallic sphere_material = metallic(albedo, fuzz);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                } else {
                    dialectric sphere_material = dialectric(1.5);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                }
            }
        }
    }
}