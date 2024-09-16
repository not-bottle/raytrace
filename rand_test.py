import math
import numpy as np

myarray = np.array([-0.7347, 1.0])

def myrandom(v):
    dp = np.dot(v, np.array([12.9898, 78.233]))
    s = math.sin(dp)
    a = s * 43758.5453
    floor_a = math.floor(a)
    return a - floor_a

def myrandrange(v, min, max):
    r = myrandom(v)
    return min + (max-min)*r

def myrandrangevec(v, min, max):
    x = myrandrange(v + np.array([0, 1]), min, max)
    y = myrandrange(v + np.array([1, 0]), min, max)
    z = myrandrange(v, min, max)
    return np.array([x, y, z])

def random_unit_vec(v):
    i = 0
    while(True):
        p = myrandrangevec(v + np.array([0, i]), -1, 1)
        lensq = np.dot(p, p)
        if (lensq <= 1):
            return p / math.sqrt(lensq)
        i += 1

def random_unit_vec2(v):
    p = myrandrangevec(v, -1, 1)
    lensq = np.dot(p, p)
    return p / math.sqrt(lensq)


print(myrandom(myarray))
print(myrandrange(myarray, -1, 1))
print(myrandrangevec(myarray, -1, 1))
print(random_unit_vec(myarray))
print(random_unit_vec2(myarray))
