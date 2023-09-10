#pragma once
#include <rayvis-utils/MathTypes.h>

// https://stackoverflow.com/a/38552664
void VoxelTrace(Double3 start, Double3 end, std::function<void(const Int3& voxel)> processVoxel)
{
#define SIGN(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))
#define FRAC0(x) (x - floor(x))
#define FRAC1(x) (1 - x + floor(x))

    double tMaxX, tMaxY, tMaxZ, tDeltaX, tDeltaY, tDeltaZ;
    Int3  voxel;

    double x1, y1, z1;  // start point
    double x2, y2, z2;  // end point
    x1 = start.x;
    x2 = end.x;
    y1 = start.y;
    y2 = end.y;
    z1 = start.z;
    z2 = end.z;

    int dx = SIGN(x2 - x1);
    if (dx != 0) {
        tDeltaX = fmin(dx / (x2 - x1), 10000000.0);
    } else {
        tDeltaX = 10000000.0;
    }
    if (dx > 0) {
        tMaxX = tDeltaX * FRAC1(x1);
    } else {
        tMaxX = tDeltaX * FRAC0(x1);
    }
    voxel.x = (int)x1;

    int dy = SIGN(y2 - y1);
    if (dy != 0) {
        tDeltaY = fmin(dy / (y2 - y1), 10000000.0);
    } else {
        tDeltaY = 10000000.0;
    }
    if (dy > 0) {
        tMaxY = tDeltaY * FRAC1(y1);
    } else {
        tMaxY = tDeltaY * FRAC0(y1);
    }
    voxel.y = (int)y1;

    int dz = SIGN(z2 - z1);
    if (dz != 0) {
        tDeltaZ = fmin(dz / (z2 - z1), 10000000.0);
    } else {
        tDeltaZ = 10000000.0;
    }
    if (dz > 0) {
        tMaxZ = tDeltaZ * FRAC1(z1);
    } else {
        tMaxZ = tDeltaZ * FRAC0(z1);
    }
    voxel.z = (int)z1;

    processVoxel(voxel);
    while (true) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                voxel.x += dx;
                tMaxX += tDeltaX;
            } else {
                voxel.z += dz;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                voxel.y += dy;
                tMaxY += tDeltaY;
            } else {
                voxel.z += dz;
                tMaxZ += tDeltaZ;
            }
        }
        if (tMaxX > 1 && tMaxY > 1 && tMaxZ > 1) {
            break;
        }
        processVoxel(voxel);
    }

#undef SIGN
#undef FRAC0
#undef FRAC1
}