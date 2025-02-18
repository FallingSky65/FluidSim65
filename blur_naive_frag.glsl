#version 330

in vec2 fragTexCoord;
out vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 screenSize;
uniform int axis;

const float blur_radius = 10.0;

void main()
{
    //if (texture(texture0, fragTexCoord).r <= 0.0) {
    //    fragColor = texture(texture0, fragTexCoord);
    //    return;
    //}

    float accumulator = 0.0;
    
    vec2 texelSize = 1.0 / screenSize;

    vec2 ds = vec2(0.0);

    if (axis == 0) { // x-axis
        ds.x = texelSize.x;
    }
    if (axis == 1) { // y-axis
        ds.y = texelSize.y;
    }

    float weight;
    float rr = blur_radius*blur_radius;
    float weightsum = 0.0;

    for (float i = -blur_radius; i <= blur_radius; i++) {
        for (float j = -blur_radius; j <= blur_radius; j++) {
            vec2 uv = fragTexCoord + texelSize * vec2(i, j);
            if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) continue;
            weight = exp(-(i*i+j*j)/(0.5*rr));
            weightsum += weight;
            if (texture(texture0, uv).r <= 0.0) {
                accumulator += weight*texture(texture0, fragTexCoord).r*1.1;
                continue;
            }
            //float depthdiff = length(texture(texture0, fragTexCoord) - texture(texture0, uv));
            //weight = weight*exp(-depthdiff*depthdiff*0.001);
            accumulator += weight*texture(texture0, uv).r;
        }
    }
    if (weightsum > 0.0) accumulator *= 1.0/weightsum;
    
    //if (count > 0) accumulator *= 1.0/count;

    fragColor = vec4(accumulator, texture(texture0, fragTexCoord).g, 0.0, 1.0);
}
