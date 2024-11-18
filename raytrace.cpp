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

#include <unistd.h>

void load_three_spheres(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO);
void load_final_scene(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO);

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
int RENDER_WIDTH = 1200;
float ASPECT_RATIO = 16.0/9.0;

int NUM_SAMPLES = 128;
uint32_t BOUNCE_LIMIT = 50;

// Other constants
const int MAX_NUM_OBJECTS = 128;
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
    int matUBOSize = 20000;
    int sphereUBOSize = 20000;

    UBO matUBO = UBO(matUBOSize);
    matUBO.bind(0, 0, -1);
    ourShader.bindUBO(matUBO, "Materials");

    UBO sphereUBO = UBO(sphereUBOSize);
    sphereUBO.bind(1, 0, -1);
    ourShader.bindUBO(sphereUBO, "Spheres");

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

    precomp1.bind(2, 0, -1);
    precomp2.bind(3, 0, -1);

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

    load_final_scene(cam, materials, objects, matUBO, sphereUBO);

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

    ourShader.setInt("num_spheres", objects.num);

    float chunk_size = 25.0f;
    float x_passes = cam.renderWidth / chunk_size;
    float y_passes = cam.renderHeight / chunk_size;

    float xmin, xmax, ymin, ymax;

    xmin = 0.0f;
    xmax = 0.0f;
    ymin = 0.0f;
    ymax = 0.0f;

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

void load_three_spheres(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO) {
    // Add Materials

    Camera::orientation uvw;

    uvw.lookfrom = point3(0, 0, 0);
    uvw.lookat = point3(0, 0, -1);
    uvw.vup = vec3(0, 1, 0);
    uvw.vfov = 90.0;
    uvw.defocus_angle = 0.0;
    uvw.focus_dist = 10;

    cam.cameraSetup(uvw);

    lambertian mat_ground = lambertian(vec3(0.8, 0.8, 0.0));
    lambertian mat_centre = lambertian(vec3(0.1, 0.2, 0.5));
    dialectric mat_left = dialectric(1.5);
    dialectric mat_left_bubble = dialectric(1.0/1.5);
    metallic mat_right = metallic(vec3(0.8, 0.6, 0.2), 0.0);

    metallic mat_right_2 = metallic(vec3(0.8, 0.2, 0.6), 0.0);
    metallic mat_right_3 = metallic(vec3(0.2, 0.6, 0.8), 0.0);
    metallic mat_right_4 = metallic(vec3(0.3, 0.5, 0.3), 0.0);

    materials.add(matUBO, mat_ground);
    materials.add(matUBO, mat_centre);
    materials.add(matUBO, mat_left);
    materials.add(matUBO, mat_right);
    materials.add(matUBO, mat_left_bubble);
    materials.add(matUBO, mat_right_2);
    materials.add(matUBO, mat_right_3);
    materials.add(matUBO, mat_right_4);

    // Add spheres

    sphere ground = sphere(100.0, vec3(0.0, -100.5, -1.0), &mat_right_4);
    sphere centre = sphere(0.5, vec3(0.0, 0.0, -1.2), &mat_right);
    sphere left = sphere(0.5, vec3(-1.0, 0.0, -1.0), &mat_right_2);
    sphere left_bubble = sphere(0.4, vec3(-1.0, 0.0, -1.0), &mat_left_bubble);
    sphere right = sphere(0.5, vec3(1.0, 0.0, -1.0), &mat_right_3);

    objects.add(sphereUBO, ground);
    objects.add(sphereUBO, centre);
    objects.add(sphereUBO, left);
    objects.add(sphereUBO, left_bubble);
    objects.add(sphereUBO, right);
}

void load_final_scene(Camera &cam, material_list &materials, hittable_list &objects, UBO matUBO, UBO sphereUBO) {
    Camera::orientation uvw;

    uvw.lookfrom = point3(13, 2, 3);
    uvw.lookat = point3(0, 0, 0);
    uvw.vup = vec3(0, 1, 0);
    uvw.vfov = 20.0;
    uvw.defocus_angle = 0.6;
    uvw.focus_dist = 10;

    cam.cameraSetup(uvw);

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

                if (choose_mat < 0.8) {
                    auto centre2 = centre + vec3(0.0, random_float(0, 0.5), 0.0);

                    auto albedo = colour::random() * colour::random();
                    lambertian sphere_material = lambertian(albedo);
                    materials.add(matUBO, sphere_material);
                    sphere spherex = sphere(0.2, centre, centre2 - centre, &sphere_material);
                    objects.add(sphereUBO, spherex);
                } else if (choose_mat < 0.95) {
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