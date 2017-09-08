void main(){
  gl_Position = MVP * vec4(in_position,1);
  vec4 sm = shadowmapMVP * vec4(in_position,1);
  out_shadowmap = sm.xyz / sm.w;
  out_world_normal = in_normal;
  out_world_position = in_position;
  out_texuv = in_texuv;
}
