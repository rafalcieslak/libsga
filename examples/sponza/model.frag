float attenuation(float r, float f, float d) {
  float denom = d / r + 1.0;
  float attenuation = 1.0 / (denom*denom);
  float t = (attenuation - f) / (1.0 - f);
  return max(t, 0.0);
}

float get_shadow(vec2 coords, vec2 offset){
  float sm = texture(shadowmap, (coords/2.0 + 0.5) + offset).x;
  if(sm < 1.0 - in_shadowmap.z + 0.001) return 1.0;
  else return 0.0;
}

// PCF
float get_shadow_filtered(vec2 c){
  float d = 1.0/textureSize(shadowmap, 0).x;
  float acc = 0;
  int count = 0, range = 3;
  for(int dx = -range; dx <= range; dx++)
    for(int dy = -range; dy <= range; dy++){
      acc += get_shadow(c, vec2(d*dx, d*dy));
      count++;
    }
  return acc/count;
}

vec3 light_color = vec3(1.0, 0.96, 0.92);
vec3 ambient_color = vec3(0.1, 0.108, 0.11);

void main(){
  vec3 light_vector = world_lightpos - in_world_position;
  float light_distance = length(light_vector);
  vec3 L = normalize(light_vector);
  vec3 N = normalize(in_world_normal);
  vec3 V = normalize(world_viewpos - in_world_position);
  float att = attenuation(2000, 0.002, light_distance);
  // Colors:
  vec3 Kd = pow(texture(diffuse, in_texuv).xyz, vec3(2.2));
  vec3 Ks = Kd; // TODO: Does this model use different colors for specular?
  vec3 Ka = Kd;
  // Lambertian diffuse
  float d = max(0.0, dot(L, N)) * att;
  // Phong specular
  vec3 R = reflect(L,N);
  float exponent = 30.0;
  float s = pow(max(0.0, dot(R, -V)), exponent) * att;

  float shadow_factor = 1.0f;
  if(in_shadowmap.x > 1.0 || in_shadowmap.x < -1.0 || in_shadowmap.y > 1.0 || in_shadowmap.y < -1.0)
    shadow_factor = 0.0f;
  else
    shadow_factor = get_shadow_filtered(in_shadowmap.xy);

  vec3 D = Kd * d * shadow_factor * light_color;
  vec3 S = Ks * s * 0.4 * shadow_factor;
  vec3 A = Ka * ambient_color;
  out_color = vec4(D + S + A, 1.0);
}
