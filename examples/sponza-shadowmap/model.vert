void main(){
  vec3 pos = in_position + vec3(0, -4, 0);
  gl_Position = MVP * vec4(pos,1);
  vec4 sm = shadowmapMVP * vec4(pos,1);
  out_shadowmap = sm.xyz / sm.w;
  out_world_normal = in_normal;
  out_world_position = pos;
  out_texuv = in_texuv;
}
