#version 410

in   vec4 vPosition;
in   vec3 vNormal;
in   vec2 vTexCoord;

uniform vec4 vColor;

// output values that will be interpretated per-fragment
out  vec4 ccolor;
out  vec4 color;
out  vec3 fN;
out  vec3 fV;
out  vec3 fL;
out vec2 texCoord;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform vec4 LightPosition;
uniform mat4 Projection;
uniform int ShadingType;
uniform float Shininess;

void main()
{
	texCoord    = vTexCoord;
	
    // Transform vertex position into camera (eye) coordinates
    vec3 pos = (ModelView * vPosition).xyz;
    
    
	if(ShadingType == 0 || ShadingType == 3){ //0: Phong, 3: Modified-Phong
		fN = (ModelView * vec4(vNormal, 0.0)).xyz; // normal direction in camera coordinates

		fV = -pos; //viewer direction in camera coordinates

		fL = LightPosition.xyz; // light direction

		if( LightPosition.w != 0.0 ) {
			fL = LightPosition.xyz - pos;  //directional light source
		}
	}else if(ShadingType == 1){ // 1: Gouraud
		vec3 L = normalize( LightPosition.xyz - pos ); //light direction
		vec3 V = normalize( -pos ); // viewer direction
		vec3 H = normalize( L + V ); // halfway vector

		// Transform vertex normal into camera coordinates
		vec3 N = normalize( ModelView * vec4(vNormal, 0.0) ).xyz;

		// Compute terms in the illumination equation
		vec4 ambient = AmbientProduct;

		float Kd = max( dot(L, N), 0.0 ); //set diffuse to 0 if light is behind the surface point
		vec4  diffuse = Kd*DiffuseProduct;

		float Ks = pow( max(dot(N, H), 0.0), Shininess );
		
		
		vec4  specular = Ks * SpecularProduct;
    
		//ignore also specular component if light is behind the surface point
		if( dot(L, N) < 0.0 ) {
			specular = vec4(0.0, 0.0, 0.0, 1.0);
		}
		color = ambient + diffuse + specular;
		color.a = 1.0;
	}

    
    
    gl_Position = Projection * ModelView * vPosition;
	ccolor = vColor;
}
