#version 410
in  vec4 color;
in  vec2 texCoord;

out vec4 fcolor;
uniform bool ShadingType;
uniform bool TextureFlag;

in  vec3 fN;
in  vec3 fL;
in  vec3 fV;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

uniform sampler2D tex;


void main() 
{ 
    if(ShadingType){
        fcolor = color;
    }else{
        vec3 N = normalize(fN);
        vec3 V = normalize(fV);
        vec3 L = normalize(fL);

        vec3 H = normalize( L + V );
        
        vec4 ambient = AmbientProduct;

        float Kd = max(dot(L, N), 0.0);
        vec4 diffuse = Kd*DiffuseProduct;
        
        float Ks = pow(max(dot(N, H), 0.0), Shininess);
        vec4 specular = Ks*SpecularProduct;

        // discard the specular highlight if the light's behind the vertex
        if( dot(L, N) < 0.0 ) {
            specular = vec4(0.0, 0.0, 0.0, 1.0);
        }


    fcolor = ambient + diffuse + specular;
        fcolor.a = 1.0;
}   
} 

