uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;

uniform vec2 texelSize0;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

void main(void)
{
	vec2 uv = varTexCoord.st;
    vec4 uniformValue = texture2D(texture3, vec2(0,0));
    vec4 color = texture2D(texture0, uv + texelSize0);
    if(uv.x > uniformValue.x)
        gl_FragColor = vec4(0,0,0,1);
    else
        gl_FragColor = color;
}
