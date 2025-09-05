#include "glhelp.h"
#include "raytrace.h"
#include "vec3.h"

const int SCREEN_WIDTH = 1600;
const int RENDER_WIDTH = 800;
const float ASPECT_RATIO = 16.0/9.0;

const int glMajorVersion = 4;
const int glMinorVersion = 3;

int main(int argc, char const *argv[])
{
    Camera cam = Camera(SCREEN_WIDTH, RENDER_WIDTH, ASPECT_RATIO);
    Context c = Context(cam.screenWidth, cam.screenHeight, glMajorVersion, glMinorVersion);
    
    if (!c.init()) exit(1);

    colour clearColour = colour(0.1, 0.3, 0.5);

    while(!c.isQuit()) {
        c.bind();
        c.clearBuffer(clearColour);
        c.event_handling();
        c.loopEnd();
    }

    c.close();

    return 0;
}
