#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "dvector3.hpp"

struct OrbitSegment;

void RenderOrbit(const OrbitSegment* orbit, int point_count, Color color);
void RenderPerfectSphere(DVector3 pos, double radius, Color color);

#endif  // RENDER_UTILS_H