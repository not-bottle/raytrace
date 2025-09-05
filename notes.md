# AABBs
The process should be:
1. Generate AABB hierarchy from the list of objects
    1. Create an interval class
    2. Create an AABB class that can be constructed from intervals
    3. Create a BBH node class, with tree branching and splitting functions
    4. Create a method to construct a BBH from a list of spheres 
2. Export it into a list format that can be contained in a UBO
    - Which method of indexing should I use? (I remember there being a neat one for binary trees that is just a small algorithm sequence, so I don't need to include explicit pointer/index info in the nodes)
    - It might be nicer to keep two lists: One that contains the BBH intervals (sorted), and one that contains the spheres (unsorted), with the leaf nodes of the BBH containing the indices of the respective sphere(s).
    - An issue with the above is that the leaf nodes have an extra parameter, which would create uneccessary empty space in the rest of the nodes in the body. I could maybe separate the leaf nodes from the rest of them?
3. The bounce function in the shader checks for ray intersection of bounding box
    - This should be pretty straightforward once everything is set up, minus the edge cases.

# Debugging the Bounding boxes
Not sure if its a memory-usage bug or a problem with the intersection code.
I think I can break it down into 3 sections:
1. A bug in the c++ code creating and passing the input values
2. A bug in the GLSL code:
    1. Problem with how the input structs are set up and handled inside the shader code
    2. Problem with the intersection calculation code
3. A bug relating to GLSL use (e.g too much memory use, some sort of overflow etc.)

Since this is complicated I'll try and work through methodically until I'm Pretty sure that each section is working fine.

Here is the todo list:

1. Make a grid-like test scene for easier debug reasoning
2. Test that the BVH input is passed correctly (sort of handles 1. and 2.1.)
3. Run intersection calculation code separately on fixed cases (and print a colour if correct)
4. Test that the queue struct works properly
5. Put this together by running some small fixed bvh cases with expected outcomes (not too rigorously bc of the randomization)
* If none of the above get it to work, look into GLSL memory usage and consider how I could break the shader down into separate pipeline steps (e.g: one shader to handle bvh calculations, then a separate one to use output values on spheres)

# More BVH enlightenment (and further stuff to explore)

So I'm still at a bit of a wall But I think the queue is the culprit. In this way I looked into Stackless BVH traversal and found a lot of research. The one that interests me the most is the hit-or-miss pointers algorithm, as it seems the easiest to add to my implementation.

I also had more of an out-of-my-depth moment seeing all the ray tracing paper talk, and was considering redoing my hobby raytracer to be more professional and utilize more optimizations. But I should probably finish this one first, to the best of my ability. My shader code is also remarkably sloppy, and I'm sure there's plenty of optimizing that can happen as is.

Anyway, here are the articles and things to explore:

Slides showing the hit-or-miss pointer method, and the stackoverflow post I got them from:
https://cs.uwaterloo.ca/%7Ethachisu/tdf2015.pdf
https://stackoverflow.com/questions/55479683/traversal-of-bounding-volume-hierachy-in-shaders

Series on building BVH-s, with a ray tracer implementation:
https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
^ Lots of other article series on this site that look like they'd be good to do.
Introduces "Whitted-style raytracing":
https://www.cs.drexel.edu/~deb39/Classes/Papers/p343-whitted.pdf

# Initial working bvh time stats
| scene |  res     | rays | chunk_size | bounce_limit | time_without_bvh | time_with_bvh | time_with_bvh_optimization |
| ----- | -------- | ---- | ---------- | ------------ | ---------------- | ------------- | -------------------------- |
| final | 1600x900 | 1    | 25x25px    | 50           | 38s              | 38s           |                            |
| final | 1600x900 | 2    | 25x25px    | 50           | 38s              | 38s           |                            |
| final | 1600x900 | 4    | 25x25px    | 50           | 38s              | 38s           |                            |
| final | 1600x900 | 8    | 25x25px    | 50           | 39s              | 38s           |                            |
| final | 1600x900 | 16   | 25x25px    | 50           | 48s              | 39s           |                            |
| final | 1600x900 | 32   | 25x25px    | 50           | 77s              | 48s           |                            |
| final | 1600x900 | 64   | 25x25px    | 50           | 140s             | 76s           |                            |
| final | 1600x900 | 128  | 25x25px    | 50           | 275s             | 138s          | 111s                       |
| final | 1600x900 | 256  | 25x25px    | 50           | 544s             | 263s          |                            |
| final | 1600x900 | 512  | 25x25px    | 50           | 1082s            | 516s          |                            |
