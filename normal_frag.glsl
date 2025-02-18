#version 330

in vec2 fragTexCoord;
out vec4 fragColor;

uniform sampler2D texture0;  // Depth buffer texture
uniform mat4 invProjection;      // Inverse Projection matrix
uniform mat4 invView;            // Inverse View matrix
uniform vec2 screenSize;
uniform vec3 viewPos;

float getDepth(vec2 uv) {
    float depth = texture(texture0, uv).r;
    if (depth <= 0.0) discard;
    if (texture(texture0, uv).g <= 0.0) discard;
    return depth;

    //return (1.0/texture(texture0, uv).r - 1.0)*4.0;
    //float depth = texture(texture0, uv).r;
    //if (depth >= 1.0) discard;
    //float ndcDepth = depth * 2.0 - 1.0;
    //vec4 clipSpacePos = vec4(fragTexCoord * 2.0 - 1.0, ndcDepth, 1.0);
    //vec4 viewSpacePos = invProjection * clipSpacePos;
    //viewSpacePos /= viewSpacePos.w;  // Perspective divide
    
    //return length(viewSpacePos);
}

vec3 getViewSpacePos(vec2 uv) {
    float depth = getDepth(uv);

    vec3 fragPos = (invProjection * vec4(uv.xy * 2.0 - vec2(1.0), depth, 1.0)).xyz;
    fragPos = normalize(fragPos) * depth;
    return fragPos;
}

void main()
{
    // Get depth value from depth buffer
    //float depth = getDepth(fragTexCoord);

    vec3 fragPos = getViewSpacePos(fragTexCoord);

    // Compute screen-space normal using depth differences
    vec2 texelSize = 1.0 / screenSize;

    vec3 ddx = getViewSpacePos(fragTexCoord + vec2(texelSize.x, 0.0)) - fragPos;
    vec3 ddx2 = fragPos - getViewSpacePos(fragTexCoord - vec2(texelSize.x, 0.0));
    if (length(ddx) > length(ddx2)) ddx = ddx2;

    vec3 ddy = getViewSpacePos(fragTexCoord + vec2(0.0, texelSize.y)) - fragPos;
    vec3 ddy2 = fragPos - getViewSpacePos(fragTexCoord - vec2(0.0, texelSize.y));
    if (length(ddy2) < length(ddy)) ddy = ddy2;

    vec3 N = normalize(cross(ddx, ddy));
    
    N = mat3(invView) * N;
    N = normalize(N);

    vec3 L = normalize(vec3(1.0, 1.0, -1.0)); // direction to light
    vec3 R = reflect(-L, N); // reflected L
    vec3 V = -normalize((invView * vec4(fragPos, 1.0)).xyz - viewPos);

    float diffuse = clamp(dot(N, L), 0.0, 1.0) * 0.7;
    float ambient = 0.3;
    float specular = pow(clamp(dot(R, V), 0.0, 1.0), 12.0);
    float fresnel = clamp(pow(1.0 - dot(N, V), 3.0), 0.0, 1.0);

    vec3 color_diffuse = vec3(0.5, 0.7, 0.9);
    vec3 color_ambient = vec3(0.5, 0.7, 0.9);
    vec3 color_specular = vec3(1.0);
    vec3 color_fresnel = vec3(1.0);

    fragColor = vec4(diffuse*color_diffuse + ambient*color_ambient + specular*color_specular + fresnel*color_fresnel, 1.0);
}
