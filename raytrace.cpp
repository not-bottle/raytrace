//#define DEBUG

#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <algorithm>
#include <memory>

#include <chrono>

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

#include "bvh.h"

#include <unistd.h>

void load_three_spheres(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO, UBO bvhUBO);
void load_final_scene(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO, UBO bvhUBO);
void load_grid_scene(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO, UBO bvhUBO);

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

int SCREEN_WIDTH = 1600;
int RENDER_WIDTH = 1600;
float ASPECT_RATIO = 16.0/9.0;

int NUM_SAMPLES = 128;
uint32_t BOUNCE_LIMIT = 50;

// Other constants
const int MAX_NUM_OBJECTS = 512;
const int SPHERE_UBO_SIZE = MAX_NUM_OBJECTS*32;

int RANDOM_ARRAY_SIZE = 8192;


int main(int argc, char* args[])
{
    Camera cam = Camera(SCREEN_WIDTH, RENDER_WIDTH, ASPECT_RATIO);
    Context c = Context(cam.screenWidth, cam.screenHeight);

    if(!c.init())
    {
        exit(1);
    }

    FrameBuffer noisegenfb = FrameBuffer(cam.renderWidth, cam.renderHeight);
    FrameBuffer renderfb = FrameBuffer(cam.renderWidth, cam.renderHeight);

    // Queries??
    /* Get maximum number of vertex attributes we can pass to a vertex shader (it's 16) */
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum Number of Vertex Attributes Supported: " << nrAttributes << std::endl;

    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &nrAttributes);
    std::cout << "Maximum Number of Uniform Blocks Supported in Fragment Shader: " << nrAttributes << std::endl;

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &nrAttributes);
    std::cout << "Maximum Uniform Block Size (bytes): " << nrAttributes << std::endl;

    VAO vao;
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
    vao.bindEBO(indices, sizeof(indices), sizeof(indices)/sizeof(unsigned int));

    // SHADER CREATION:
    Shader ourShader("shaders/testVertex.vs", "shaders/testFragment.fs");
    Shader noiseShader("shaders/testVertex.vs", "shaders/prng.fs");
    Shader texShader("shaders/texVertex.vs", "shaders/texFragment.fs");

    // Set up UBO data

    // MATERIALS
    int matUBOSize = 24576;
    int sphereUBOSize = 24576;
    int bvhUBOSize = 3*sphereUBOSize; // Since bvh is a btree, and spheres are the leaf nodes, 
                                      // should not exceed 2*sphereUBOSize
                                      // (Note: spheres are (currently) 48 bytes, bvh nodes are 32)

    UBO matUBO = UBO(matUBOSize);
    matUBO.bind(0, 0, -1);
    ourShader.bindUBO(matUBO, "Materials");

    UBO sphereUBO = UBO(sphereUBOSize);
    sphereUBO.bind(1, 0, -1);
    ourShader.bindUBO(sphereUBO, "Spheres");

    UBO bvhUBO = UBO(bvhUBOSize);
    bvhUBO.bind(2, 0, -1);
    ourShader.bindUBO(bvhUBO, "BVH");

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

    int precomp1Size = sizeof(rand_unit_vectors) + sizeof(rand_unit_disks);
    int precomp2Size = sizeof(rand_squares) + sizeof(rand_vectors);

    UBO precomp1 = UBO(precomp1Size);
    UBO precomp2 = UBO(precomp2Size);

    precomp1.bind(3, 0, -1);
    precomp2.bind(4, 0, -1);

    ourShader.bindUBO(precomp1, "Precomp_1");
    ourShader.bindUBO(precomp2, "Precomp_2");

    precomp1.sub(rand_unit_vectors, sizeof(rand_unit_vectors), 0);
    precomp1.sub(rand_unit_disks, sizeof(rand_unit_disks), sizeof(rand_unit_vectors));

    precomp2.sub(rand_squares, sizeof(rand_squares), 0);
    precomp2.sub(rand_vectors, sizeof(rand_vectors), sizeof(rand_squares));

    // Set up scene

    // Raytracing camera setup

    material_list materials = material_list();
    hittable_list objects = hittable_list();

    load_final_scene(cam, materials, objects, matUBO, sphereUBO, bvhUBO);

    colour clearcolour = colour(0.2f, 0.3f, 0.3f);
    // NOISEGEN PASS:

    noisegenfb.bind();

    c.clearBuffer(clearcolour);
    noiseShader.use();
    noiseShader.setUint("timevalue", 3568u);

    vao.draw();

    // UPSCALE PASS:

    renderfb.bind();
    ourShader.use();
    ourShader.bindTex(0, "randTexture");
    ourShader.bindTex(1, "screenTexture");

    noisegenfb.bindTexture(0);
    renderfb.bindTexture(1);
    c.clearBuffer(clearcolour);

    // Shader uniforms

    uint32_t timeValue = SDL_GetTicks();
    ourShader.setUint("time_u32t", timeValue);

    ourShader.setInt("num_samples", NUM_SAMPLES);
    ourShader.setUint("bounce_limit", BOUNCE_LIMIT);

    ourShader.setVec3("delta_u", cam.uvw.delta_u);
    ourShader.setVec3("delta_v", cam.uvw.delta_v);
    ourShader.setVec3("camera_origin", cam.uvw.lookfrom);
    ourShader.setVec3("viewport_top_left", cam.uvw.viewport_top_left);

    ourShader.setFloat("defocus_angle", cam.uvw.defocus_angle);
    ourShader.setVec3("defocus_disk_u", cam.uvw.defocus_disk_u);
    ourShader.setVec3("defocus_disk_v", cam.uvw.defocus_disk_v);

    ourShader.setInt("num_spheres", objects.objects.size());

    float chunk_size = 100.0f;
    float x_passes = cam.renderWidth / chunk_size;
    float y_passes = cam.renderHeight / chunk_size;

    float xmin, xmax, ymin, ymax;

    xmin = 0.0f;
    xmax = 0.0f;
    ymin = 0.0f;
    ymax = 0.0f;

    auto a = std::chrono::high_resolution_clock::now();

    for (int j=0; j < y_passes; j++)
    {
        renderfb.bind();
        ourShader.use();
        ourShader.bindTex(0, "randTexture");
        ourShader.bindTex(1, "screenTexture");

        noisegenfb.bindTexture(0);
        renderfb.bindTexture(1);   

        ymin = ymax;
        ymax = ymin + chunk_size;

        ourShader.setFloat("Y_MIN", ymin);
        ourShader.setFloat("Y_MAX", ymax);

        for (int i=0; i < x_passes; i++) 
        {
            if (c.isQuit()) break;  

            renderfb.bind();
            ourShader.use();
            ourShader.bindTex(0, "randTexture");
            ourShader.bindTex(1, "screenTexture");

            noisegenfb.bindTexture(0);
            renderfb.bindTexture(1);      

            xmin = xmax;
            xmax = xmin + chunk_size;
            ourShader.setFloat("X_MIN", xmin);
            ourShader.setFloat("X_MAX", xmax);

            vao.draw();

            // Render partially to screen
            c.bind();
            texShader.use();
            texShader.bindTex(0, "screenTexture");
            renderfb.bindTexture(0);

            c.clearBuffer(clearcolour);
            
            vao.draw();

            c.event_handling();
            c.loopEnd();
        }

        xmin = 0.0f;
        xmax = 0.0f;
    }

    auto b = std::chrono::high_resolution_clock::now();

    std::cout << "Render time: " << std::chrono::duration_cast<std::chrono::seconds>(b - a).count() << "s" <<std::endl;

    while (!c.isQuit())
    {
        c.bind();
        texShader.use();
        texShader.bindTex(0, "screenTexture");
        renderfb.bindTexture(0);

        c.clearBuffer(clearcolour);
        
        vao.draw();

        c.event_handling();
        c.loopEnd();
    }
    
    c.close();

    return 0;
}

void load_grid_scene(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO, UBO bvhUBO) {
    // Add Materials

    Camera::orientation uvw;

    uvw.lookfrom = point3(0, 0, 0);
    uvw.lookat = point3(0, 0, -1);
    uvw.vup = vec3(0, 1, 0);
    uvw.vfov = 90.0;
    uvw.defocus_angle = 0.0;
    uvw.focus_dist = 10;

    cam.cameraSetup(uvw);

    auto mat_ground = std::make_shared<material>(lambertian(vec3(0.8, 0.8, 0.0)));
    auto mat_l1 = std::make_shared<material>(lambertian(vec3(0.1, 0.2, 0.5)));
    auto mat_l2 = std::make_shared<material>(lambertian(vec3(0.5, 0.2, 0.1)));
    auto mat_r1 = std::make_shared<material>(lambertian(vec3(0.1, 0.5, 0.2)));
    auto mat_r2 = std::make_shared<material>(lambertian(vec3(0.5, 0.1, 0.2)));

    materials.add(mat_ground);
    materials.add(mat_l1);
    materials.add(mat_l2);
    materials.add(mat_r1);
    materials.add(mat_r2);

    // Add spheres

    auto ground = std::make_shared<sphere>(sphere(100.0, vec3(0.0, -100.5, -1.0), mat_ground));
    auto l1 = std::make_shared<sphere>(sphere(0.5, vec3(-0.5, -0.5, -2.0), mat_l1));
    auto l2 = std::make_shared<sphere>(sphere(0.5, vec3(-0.5, 0.5, -2.0), mat_l2));
    auto r1 = std::make_shared<sphere>(sphere(0.5, vec3(0.5, -0.5, -2.0), mat_r1));
    auto r2 = std::make_shared<sphere>(sphere(0.5, vec3(0.5, 0.5, -2.0), mat_r2));

    //objects.add(ground);
    objects.add(l1);
    objects.add(l2);
    objects.add(r1);
    objects.add(r2);

    std::cout << objects.bounding_box();
    bvh_node bvh = bvh_node(objects);
    std::cout << bvh.bounding_box();
    objects.toUBO(sphereUBO);
    materials.toUBO(matUBO);
    bvh.toUBO(bvhUBO, 0);
}

void load_three_spheres(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO, UBO bvhUBO) {
    // Add Materials

    Camera::orientation uvw;

    uvw.lookfrom = point3(0, 0, 0);
    uvw.lookat = point3(0, 0, -1);
    uvw.vup = vec3(0, 1, 0);
    uvw.vfov = 90.0;
    uvw.defocus_angle = 0.0;
    uvw.focus_dist = 10;

    cam.cameraSetup(uvw);

    auto mat_ground = std::make_shared<material>(lambertian(vec3(0.8, 0.8, 0.0)));
    auto mat_centre = std::make_shared<material>(lambertian(vec3(0.1, 0.2, 0.5)));
    auto mat_left = std::make_shared<material>(dialectric(1.5));
    auto mat_left_bubble = std::make_shared<material>(dialectric(1.0/1.5));
    auto mat_right = std::make_shared<material>(metallic(vec3(0.8, 0.6, 0.2), 0.0));

    materials.add(mat_ground);
    materials.add(mat_centre);
    materials.add(mat_left);
    materials.add(mat_right);
    materials.add(mat_left_bubble);

    // Add spheres

    auto ground = std::make_shared<sphere>(sphere(100.0, vec3(0.0, -100.5, -1.0), mat_ground));
    auto centre = std::make_shared<sphere>(sphere(0.5, vec3(0.0, 0.0, -1.2), mat_centre));
    auto centre2 = std::make_shared<sphere>(sphere(0.5, vec3(0.0, 1.0, -1.2), mat_centre));
    auto left = std::make_shared<sphere>(sphere(0.5, vec3(-1.0, 0.0, -1.0), mat_right));
    auto left_bubble = std::make_shared<sphere>(sphere(0.5, vec3(-1.5, 0.0, -1.0), mat_centre));
    auto right = std::make_shared<sphere>(sphere(0.5, vec3(1.0, 0.0, -1.0), mat_right));

    objects.add(ground);
    objects.add(centre);
    objects.add(centre2);
    objects.add(left);
    ///objects.add(left_bubble);
    objects.add(right);

    std::cout << objects.bounding_box();
    bvh_node bvh = bvh_node(objects);
    std::cout << bvh.bounding_box();
    objects.toUBO(sphereUBO);
    materials.toUBO(matUBO);
    bvh.toUBO(bvhUBO, 0);
}

void load_final_scene(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO, UBO bvhUBO) {
    Camera::orientation uvw;

    uvw.lookfrom = point3(13, 2, 3);
    uvw.lookat = point3(0, 0, 0);
    uvw.vup = vec3(0, 1, 0);
    uvw.vfov = 20.0;
    uvw.defocus_angle = 0.6;
    uvw.focus_dist = 10;

    cam.cameraSetup(uvw);

    auto ground_material = std::make_shared<material>(lambertian(colour(0.5, 0.5, 0.5)));
    auto ground = std::make_shared<sphere>(sphere(1000, point3(0, -1000, 0), ground_material));
    materials.add(ground_material);
    objects.add(ground);

    auto material1 = std::make_shared<material>(dialectric(1.5));
    auto material2 = std::make_shared<material>(lambertian(colour(0.4, 0.2, 0.1)));
    auto material3 = std::make_shared<material>(metallic(colour(0.7, 0.6, 0.5), 0.0));

    materials.add(material1);
    materials.add(material2);
    materials.add(material3);

    auto sphere1 = std::make_shared<sphere>(sphere(1.0, point3(0, 1, 0), material1));
    auto sphere2 = std::make_shared<sphere>(sphere(1.0, point3(-4, 1, 0), material2));
    auto sphere3 = std::make_shared<sphere>(sphere(1.0, point3(4, 1, 0), material3));

    objects.add(sphere1);
    objects.add(sphere2);
    objects.add(sphere3);

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_float();
            point3 centre(a + 0.9*random_float(), 0.2, b + 0.9*random_float());

            if ((centre - point3(4, 0.2, 0)).length() > 0.9) {

                if (choose_mat < 0.8) {
                    auto centre2 = centre + vec3(0.0, random_float(0, 0.3), 0.0);
                    auto albedo = colour::random() * colour::random();
                    auto sphere_material = std::make_shared<material>(lambertian(albedo));
                    materials.add(sphere_material);
                    auto spherex = std::make_shared<sphere>(sphere(0.2, centre, sphere_material));
                    objects.add(spherex);
                } else if (choose_mat < 0.95) {
                    auto albedo = colour::random(0.5, 1);
                    auto fuzz = random_float(0, 0.5);
                    auto sphere_material = std::make_shared<material>(metallic(albedo, fuzz));
                    materials.add(sphere_material);
                    auto spherex = std::make_shared<sphere>(sphere(0.2, centre, sphere_material));
                    objects.add(spherex);
                } else {
                    auto sphere_material = std::make_shared<material>(dialectric(1.5));
                    materials.add(sphere_material);
                    auto spherex = std::make_shared<sphere>(sphere(0.2, centre, sphere_material));
                    objects.add(spherex);
                }
            }
        }
    }

    std::cout << objects.bounding_box();
    bvh_node bvh = bvh_node(objects);
    std::cout << bvh.bounding_box();
    objects.toUBO(sphereUBO);
    materials.toUBO(matUBO);
    bvh.toUBO(bvhUBO, 0);
}