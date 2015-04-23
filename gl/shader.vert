attribute vec2 aVertex;
varying vec2 vTexCoord;

void main(void)
{
    gl_Position = vec4(aVertex,0.0,1.0);
    vTexCoord = aVertex*vec2(0.5,0.5) + vec2(0.5,0.5);
}
