
void PointLight(in vec3 light_position,
                in vec3 light_attenuation,
                in vec3 eye,
                in vec3 ecPosition3,
                in vec3 normal,
                inout vec4 ambient,
                inout vec4 diffuse,
                inout vec4 specular)
{
  float nDotVP;
  float nDotHV;
  float pf;  
  float att;
  float d;
  vec3 VP;
  vec3 halfvec;

  VP = light_position - ecPosition3;
  d = length(VP);

  VP = normalize(VP);

  att = 1.0 / (light_attenuation.x +
               light_attenuation.y * d +
               light_attenuation.z * d * d);

  halfvec = normalize(VP + eye);
  nDotVP = max(0.0, dot(normal, VP));
  nDotHV = max(0.0, dot(normal, halfvec));

  float shininess = gl_FrontMaterial.shininess;

  if (nDotVP == 0.0)
    pf = 0.0;
  else
    pf = pow(nDotHV, shininess);

  ambient *= att;
  diffuse *= nDotVP * att;
  specular *= pf * att;
}

