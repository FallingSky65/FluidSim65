#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragCenter;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

// Output fragment color
out vec4 finalColor;

uniform float ball_radius;
uniform vec3 viewPos;

void main()
{
    vec3 N;
    N.xy = fragTexCoord*2.0 - 1.0;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;
    N.z = sqrt(1.0 - r2);

    //N = mat3(viewMatrix) * N;

    vec3 hitPos = ((viewMatrix * vec4(fragCenter, 1.0)).xyz + N*ball_radius);
    float depth = length(hitPos - (viewMatrix * vec4(viewPos, 1.0)).xyz);

    // Texel color fetching from texture sampler
    //vec4 texelColor = texture(texture0, fragTexCoord);
    //if (length(fragPosition - fragCenter) > ball_radius) discard;

    //float a = length(viewPos - fragCenter);
    //float b = length(fragPosition - fragCenter);
    //float r = ball_radius;

    //float A = a*a + b*b;
    //float B = -2.0*b*b;
    //float C = b*b - r*r;

    //float discriminator = B*B - 4.0*A*C;
    //if (discriminator < 0.0) discard;

    //float x = (-B + sqrt(discriminator)) * a/(2.0*A);
    //float y = sqrt(r*r - x*x);

    //vec3 hitPos = fragCenter + x*normalize(viewPos - fragCenter) + y*normalize(fragPosition - fragCenter);
    //vec3 hitPos = fragPosition - length(fragPosition - fragCenter)*normalize(viewPos - fragPosition);

    //finalColor = texelColor*colDiffuse*fragColor;
    //float depth = sqrt((a-x)*(a-x) + y*y);
    //float depth = length(hitPos - viewPos);
    //float depth = length(fragPosition - viewPos);
    //float depth = length(fragCenter - viewPos) - dot(fragPosition - fragCenter, normalize(viewPos - fragCenter)*ball_radius);

    //vec4 clipSpacePos = projMatrix * viewMatrix * vec4(hitPos, 1.0);
    vec4 clipSpacePos = projMatrix * vec4(hitPos, 1.0);

    gl_FragDepth = (clipSpacePos.z / clipSpacePos.w) * 0.5 + 0.5;
    
    //finalColor = vec4(vec3(1.0/(depth*0.25+1.0)), 1.0);
    finalColor = vec4(vec3(depth), 1.0);
    //vec3 normal = normalize(hitPos - fragCenter);
    //finalColor = vec4(fragColor.rgb * (clamp(dot(N, normalize(vec3(1.0))), 0.0, 1.0)*0.7 + 0.3), 1.0);
}
