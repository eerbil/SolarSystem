#version 410
in vec4 vPosition;
in vec3 vNormal;
uniform vec4 Color;
out vec4 color;
out vec3 fN;
out vec3 fV;
out vec3 fL1;

uniform vec4 AmbientProduct1, DiffuseProduct1, SpecularProduct1;

uniform mat4 ModelView;
uniform mat4 GlobalModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform float Shininess;
uniform float Distance;
uniform float Sun;

void main()
{
    if(Sun == 1){
        gl_Position = Projection * ModelView * vPosition;
        color = Color;
    } else {
        fN = (GlobalModelView * vec4(vNormal, 0.0)).xyz; // normal direction in camera coordinates
        
        fV = (GlobalModelView * vPosition).xyz; //viewer direction in camera coordinates
        
        fL1 = LightPosition1.xyz; // light direction
        
        if( LightPosition1.w != 0.0 ) {
            fL1 = LightPosition1.xyz - fV;  //directional light source
        }
        
        gl_Position = Projection * ModelView * vPosition;
    }
}