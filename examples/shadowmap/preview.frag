void main(){
  float d = texture(shadowmap, sgaViewportCoords).x;
  out_color = vec4(d,d,d,1);
}
