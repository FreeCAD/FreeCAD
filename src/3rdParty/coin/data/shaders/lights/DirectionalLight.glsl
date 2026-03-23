
void DirectionalLight(in vec3 light_vector,
                      in vec3 light_halfVector,
                      in vec3 normal,
                      inout vec4 diffuse,
                      inout vec4 specular)
{
  float nDotVP; // normal . light direction
  float nDotHV; // normal . light half vector
  float pf;     // power factor

  nDotVP = max(0.0, dot(normal, light_vector));
  nDotHV = max(0.0, dot(normal, light_halfVector));

  float shininess = gl_FrontMaterial.shininess;
  if (nDotVP == 0.0)
    pf = 0.0;
  else
    pf = pow(nDotHV, shininess);

  diffuse *= nDotVP;  
  specular *= pf;
}

