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