#version 330

in vec2 fragTexCoord;
out vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 screenSize;
uniform int axis;

const float blur_radius = 15.0;

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
    float w0 = 0.5135/pow(blur_radius, 0.96);
    float rr = blur_radius*blur_radius;
    float weightsum = 0.0;

    for (float i = -blur_radius; i <= blur_radius; i++) {
        if (fragTexCoord.x + ds.x*i < 0.0 || fragTexCoord.x + ds.x*i > 1.0) continue;
        if (fragTexCoord.y + ds.y*i < 0.0 || fragTexCoord.y + ds.y*i > 1.0) continue;
        weight = exp((-i*i)/(0.5*rr));
        weightsum += weight;
        if (texture(texture0, fragTexCoord + ds*i).r <= 0.0) {
            accumulator += weight*10000.0;
        }
        //float depthdiff = length(texture(texture0, fragTexCoord) - texture(texture0, fragTexCoord + ds*i));
        //weight = *exp(-depthdiff*depthdiff*0.05);
        accumulator += weight*texture(texture0, fragTexCoord + ds*i).r;
    }
    if (weightsum > 0.0) accumulator *= 1.0/weightsum;
    
    //if (count > 0) accumulator *= 1.0/count;

    fragColor = vec4(accumulator, texture(texture0, fragTexCoord).g, 0.0, 1.0);
}
