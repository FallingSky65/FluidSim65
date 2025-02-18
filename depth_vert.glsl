#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
//in vec4 vertexColor;      // Not required

in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragCenter;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main()
{
    vec3 eye = vec3(
        instanceTransform[1][0],
        instanceTransform[1][1],
        instanceTransform[1][2]
    );

    vec3 center = vec3(
        instanceTransform[0][0],
        instanceTransform[0][1],
        instanceTransform[0][2]
    ); 

    vec3 yprime = normalize(eye - center);
    vec3 xprime = normalize(cross(yprime, vec3(0.0, 1.0, 0.0)));
    vec3 zprime = normalize(cross(xprime, yprime));
    

    mat4 transform = mat4(
        xprime, 0.0,
        yprime, 0.0,
        zprime, 0.0,
        center, 1.0
    );
    // Send vertex attributes to fragment shader
    fragPosition = vec3(transform*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vec4(
        instanceTransform[2][0],
        instanceTransform[2][1],
        instanceTransform[2][2],
        1.0
    );
    fragCenter = center;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position, note that we multiply mvp by instanceTransform
    gl_Position = mvp*transform*vec4(vertexPosition, 1.0);
}
