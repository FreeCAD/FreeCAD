
float DirSpotLight(in vec3 dir,
                   in vec3 light_position,
                   in vec3 eye,
                   in vec3 ecPosition3,
                   in vec3 normal,
                   inout vec4 diffuse,
                   inout vec4 specular)
{
  float nDotVP;
  float nDotHV;
  float pf;
  vec3 hv = normalize(eye + dir);
  nDotVP = max(0.0, dot(normal, dir));
  nDotHV = max(0.0, dot(normal, hv));
  float shininess = gl_FrontMaterial.shininess;
  if (nDotVP == 0.0)
    pf = 0.0;
  else
    pf = pow(nDotHV, shininess);

  diffuse *= nDotVP;
  specular *= pf;
  return length(light_position - ecPosition3);
}
