#include "RubyScene.h"

namespace Ruby
{
    int TestAABBAABB(AABB& a, AABB& b)
    {
        if (fabsf(a.c.x - b.c.x) > (a.r.x + b.r.x)) return 0;
        if (fabsf(a.c.y - b.c.y) > (a.r.y + b.r.y)) return 0;
        if (fabsf(a.c.z - b.c.z) > (a.r.z + b.r.z)) return 0;
        return 1;
    }


}