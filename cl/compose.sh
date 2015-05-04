#!/bin/sh

> kernel.cl
cat camera.cl     >> kernel.cl
cat ray.cl        >> kernel.cl
cat hit.cl        >> kernel.cl
cat hit_info.cl   >> kernel.cl
cat random.cl     >> kernel.cl
cat start.cl      >> kernel.cl
cat matrix.cl     >> kernel.cl
cat geometry.cl   >> kernel.cl
cat solve.cl      >> kernel.cl
cat analysis.cl   >> kernel.cl
cat intersect.cl  >> kernel.cl
cat produce.cl    >> kernel.cl
cat draw.cl       >> kernel.cl
cat sweep.cl      >> kernel.cl
