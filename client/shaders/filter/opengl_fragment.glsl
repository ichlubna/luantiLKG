uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec2 texelSize0;

const int MODE_QUILT = 0;
const int MODE_HOLO = 1;
const int MODE_HOLO_2D = 2;

int iteration;
float holoTilt;
float holoPitch;
float holoCenter;
float holoViewPortionElement;
float holoSubp;
int viewCount;
int holoCols;
int holoRows;
int mode;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

int uniformCounter = 0;
float nextUniform()
{
    return float(texelFetch(texture1, ivec2(uniformCounter++,0), 0));
}

void loadUniforms()
{
    iteration = int(nextUniform());
    mode = int(nextUniform());
    holoTilt = nextUniform();
    holoPitch = nextUniform();
    holoCenter = nextUniform(); 
    holoViewPortionElement = nextUniform();
    holoSubp = nextUniform();
    viewCount = int(nextUniform());
    holoCols = int(nextUniform());
    holoRows = int(nextUniform());
}

void createQuilt()
{
    float textureSpaceRow = 1.0f/holoRows;
    float textureSpaceCol = 1.0f/holoCols;
    int row = iteration / holoCols;
    int col = iteration % holoCols;
    vec2 viewStart = vec2(textureSpaceCol * col, textureSpaceRow * row);
    vec2 viewEnd = viewStart + vec2(textureSpaceCol, textureSpaceRow);

    if(varTexCoord.x < viewStart.x || varTexCoord.x > viewEnd.x || varTexCoord.y < viewStart.y || varTexCoord.y > viewEnd.y)
    {
        discard;
        return;
    }

    vec2 normalizedViewCoords = (varTexCoord - viewStart) / (viewEnd - viewStart);
    gl_FragColor = texture2D(texture0, normalizedViewCoords);
}

vec2 texArr(vec3 uvz)
{
    int viewsCount = holoCols * holoRows;
    float z = floor(uvz.z * viewsCount);
    float x = (mod(z, holoCols) + uvz.x) / holoCols;
    float y = (floor(z / holoCols) + uvz.y) / holoRows;
    return vec2(x, y) * vec2(holoViewPortionElement, holoViewPortionElement);
}

void createHoloView()
{
    float invView = 1.0f;
    int ri = 0;
    int bi = 2;

    vec2 texCoords = varTexCoord;
    vec3 nuv = vec3(texCoords.xy, 0.0);

    vec4 rgb[3];
    for (int i=0; i < 3; i++) 
    {
        nuv.z = (texCoords.x + i * holoSubp + texCoords.y * holoTilt) * holoPitch - holoCenter;
        nuv.z = mod(nuv.z + ceil(abs(nuv.z)), 1.0);
        nuv.z = (1.0 - invView) * nuv.z + invView * (1.0 - nuv.z);
        vec2 coords = texArr(nuv);
        rgb[i] = texture2D(texture0, coords);
    }

    vec4 color = vec4(rgb[ri].r, rgb[1].g, rgb[bi].b, 1.0);
    gl_FragColor = color;
}

void main(void)
{
    loadUniforms();
    if(mode == MODE_QUILT)
        createQuilt();
    else if(mode == MODE_HOLO)
        createHoloView();
    else if(mode == MODE_HOLO_2D)
        gl_FragColor = texture2D(texture0, varTexCoord);
}
