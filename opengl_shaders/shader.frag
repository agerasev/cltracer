uniform sampler2D uTexture;
varying vec2 vTexCoord;

void main(void)
{
    gl_FragColor = texture2D(uTexture,vTexCoord);
}
