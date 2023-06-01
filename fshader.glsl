// Per-fragment interpolated values from the vertex shader

//Since normals are fixed for a given face of the cube in this example, fN per-fragment interpolation yields fixed values. Per-fragment interpolation of fL and fV however gives smoothly varying values through faces.
#version 410

in  vec3 fN;
in  vec3 fL;
in  vec3 fV;
in  vec4 ccolor;
in  vec4 color;

in  vec2 texCoord;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform vec4 LightPosition;
uniform float Shininess;
uniform int ShadingType;

uniform int TextureFlag;
uniform sampler2D tex;


out vec4 fcolor;

void main() 
{ 
       
	if(ShadingType == 0 || ShadingType == 3){ //0: Phong, 3: Modified-Phong
		// Normalize the input lighting vectors
        vec3 N = normalize(fN);
        vec3 V = normalize(fV);
        vec3 L = normalize(fL);

        vec3 H = normalize( L + V );
		vec3 R = reflect(-L, N);
        
        vec4 ambient = AmbientProduct;
		
		
		float Ks;
		vec4 specular;
		
		if(ShadingType == 0){
			Ks = pow(max(dot(V, R), 0.0), Shininess);
			specular = Ks*SpecularProduct;
		}else if(ShadingType == 3){
			Ks = pow(max(dot(N, H), 0.0), Shininess);
			specular = Ks*SpecularProduct;
		}

        float Kd = max(dot(L, N), 0.0);
        vec4 diffuse = Kd*DiffuseProduct;
        
        

        // discard the specular highlight if the light's behind the vertex
        if( dot(L, N) < 0.0 ) {
            specular = vec4(0.0, 0.0, 0.0, 1.0);
        }

        
		
		if(TextureFlag == 1){
			fcolor = texture(tex, texCoord) + ambient + diffuse + specular;
			fcolor.a = 1.0;
		}else{
			fcolor = ccolor + ambient + diffuse + specular;
			fcolor.a = 1.0;
		}
		
	}else if (ShadingType == 1){ // 1: Gouraud
	
		if(TextureFlag == 1){
			fcolor = texture(tex, texCoord) + color;
		}else{
			fcolor = ccolor + color;
		}
	}else { //2: Off
		if(TextureFlag == 1){
			fcolor = texture(tex, texCoord);
		}else{
			fcolor = ccolor;
		}
	}
	
	

} 

