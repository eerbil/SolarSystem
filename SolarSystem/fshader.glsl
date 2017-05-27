#version 410
in vec4 color;
out vec4 fragColor;

in vec3 fN;
in vec3 fL1;
in vec3 fV;

uniform vec4 AmbientProduct1, DiffuseProduct1, SpecularProduct1;
uniform mat4 ModelView;
uniform vec4 LightPosition1;
uniform float Shininess;
uniform float Distance;
uniform float Sun;
uniform float Star;

void main()
{
    if(Sun==1){
        fragColor = color;
    }
    else if(Star==1){
        fragColor = color;
    }
    else {
        // Normalize the input lighting vectors
        vec3 N = normalize(fN);
        vec3 V = normalize(fV);
        vec3 L1 = normalize(fL1);
        
        vec3 H1 = normalize( L1 + V );
        
        vec4 ambient1 = AmbientProduct1;
        
        float Kd1 = max(dot(L1, N), 0.0);
        vec4 diffuse1 = Kd1*DiffuseProduct1;
        
        float Ks1 = pow(max(dot(N, H1), 0.0), Shininess);
        vec4 specular1 = Ks1*SpecularProduct1;
        
        // discard the specular highlight if the light's behind the vertex
        if( dot(L1, N) < 0.0 ) {
            specular1 = vec4(0.0, 0.0, 0.0, 1.0);
        }
        
        float distanceCoefficient = 1 / (1 + (1 * Distance) + (0 * Distance * Distance));
        
        fragColor = ambient1 + distanceCoefficient * (diffuse1 + specular1);
        fragColor.a = 1.0;
    }
    
}

