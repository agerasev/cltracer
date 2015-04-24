#!/bin/sh

> kernel.cl
cat camera.cl     >> kernel.cl
cat ray.cl        >> kernel.cl
cat hit.cl        >> kernel.cl
cat hit_info.cl   >> kernel.cl
cat random.cl     >> kernel.cl
cat start.cl      >> kernel.cl
cat intersect.cl  >> kernel.cl
cat produce.cl    >> kernel.cl
cat draw.cl       >> kernel.cl
cat compact.cl    >> kernel.cl
cat sweep_up.cl   >> kernel.cl
cat sweep_down.cl >> kernel.cl
cat expand.cl     >> kernel.cl
