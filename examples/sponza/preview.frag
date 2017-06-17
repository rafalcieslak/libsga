void main(){
  float d = texture(shadowmap, sgaWindowCoords).x;
  out_color = vec4(d,d,d,1);
}
