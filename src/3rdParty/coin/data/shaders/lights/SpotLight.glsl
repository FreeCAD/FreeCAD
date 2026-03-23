
float SpotLight(in vec3 light_position,
                in vec3 light_attenuation,
                in vec3 light_spotDirection,
                in float light_spotExponent,
                in float light_spotCosCutOff,
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
  float spotDot;
  float spotAtt;
  float d;
  vec3 VP;
  vec3 halfvec;

  VP = light_position - ecPosition3;
  d = length(VP);
  VP = normalize(VP);

  att = 1.0 / (light_attenuation.x +
               light_attenuation.y * d +
               light_attenuation.z * d * d);

  spotDot = dot(-VP, light_spotDirection);

  // need to read this variable outside the if statement to work around ATi driver issues
  float spotexp = light_spotExponent;

  if (spotDot < light_spotCosCutOff)
    spotAtt = 0.0;
  else
    spotAtt = pow(spotDot, spotexp);

  att *= spotAtt;

  halfvec = normalize(VP + eye);
  nDotVP = max(0.0, dot(normal, VP));
  nDotHV = max(0.0, dot(normal, halfvec));

  // need to read this variable outside the if statement to work around ATi driver issues
  float shininess =  gl_FrontMaterial.shininess;

  if (nDotVP == 0.0)
    pf = 0.0;
  else
    pf = pow(nDotHV, shininess);

  ambient *= att;
  diffuse *= nDotVP * att;
  specular *= pf * att;

  return d;
}

