#include <cmath>

#include <sga.hpp>

struct __attribute__((packed)) CustomData{
  float position[2];
  float fragPos[2];
};

std::vector<CustomData> vertices = {
  { {-1, -1 }, { 0, 0 }, },
  { { 3, -1 }, { 2, 0 }, },
  { {-1,  3 }, { 0, 2 }, },
};

extern std::string fragShaderSource;

int main(){
  sga::init(sga::VerbosityLevel::Debug, sga::ErrorStrategy::MessageThrow);
  auto window = sga::Window::create(800, 600, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO::create(
    {sga::DataType::Float2,
     sga::DataType::Float2}, 3);
  
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main()
    {
      gl_Position = vec4(inVertex, 0, 1);
      outFragPos = inFragPos;
    }
  )");

  auto fragShader = sga::FragmentShader::createFromSource(
    sga::FragmentShader::importShaderToyShader(fragShaderSource));

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  vertShader->addInput(sga::DataType::Float2, "inFragPos");
  vertShader->addOutput(sga::DataType::Float2, "outFragPos");

  fragShader->addInput(sga::DataType::Float2, "inFragPos");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}


// This is UNMODIFIED shader from https://www.shadertoy.com/view/Mss3R8

std::string fragShaderSource = R"(
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.


// learn more here: // http://www.iquilezles.org/www/articles/distancefractals/distancefractals.htm	

float calc( vec2 p, float time )
{
	p = -1.0 + 2.0*p;
	p.x *= iResolution.x/iResolution.y;

	float ltime = 0.5-0.5*cos(time*0.12);
    float zoom = pow( 0.9, 100.0*ltime );
	float an = 2.0*ltime;
	p = mat2(cos(an),sin(an),-sin(an),cos(an))*p;
	vec2 ce = vec2( 0.2655,0.301 );
	ce += zoom*0.8*cos(4.0+4.0*ltime);
	p = ce + (p-ce)*zoom;
	vec2 c = vec2( -0.745, 0.186 ) - 0.045*zoom*(1.0-ltime);
	
	vec2 z = p;
   
#if 0
    // full derivatives version
	vec2 dz = vec2( 1.0, 0.0 );
	for( int i=0; i<256; i++ )
	{
		dz = 2.0*vec2(z.x*dz.x-z.y*dz.y, z.x*dz.y + z.y*dz.x );
        z = vec2( z.x*z.x - z.y*z.y, 2.0*z.x*z.y ) + c;
		if( dot(z,z)>200.0 ) break;
	}
	float d = sqrt( dot(z,z)/dot(dz,dz) )*log(dot(z,z));

#else
    // only derivative length version
    float ld2 = 1.0;
    float lz2 = dot(z,z);
    for( int i=0; i<256; i++ )
	{
        z = vec2( z.x*z.x - z.y*z.y, 2.0*z.x*z.y ) + c;
        ld2 *= 4.0*lz2;
        lz2 = dot(z,z);
		if( lz2>200.0 ) break;
	}
    float d = sqrt(lz2/ld2)*log(lz2);

#endif
    
	return pow( clamp( (150.0/zoom)*d, 0.0, 1.0 ), 0.5 );
}

	
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	#if 0
	float scol = calc( fragCoord.xy/iResolution.xy, iGlobalTime );
    #else

    float scol = 0.0;
	for( int j=0; j<2; j++ )
	for( int i=0; i<2; i++ )
	{
		vec2 of = -0.5 + vec2( float(i), float(j) ) / 2.0;
	    scol += calc( (fragCoord.xy+of)/iResolution.xy, iGlobalTime );
	}
	scol *= 0.25;

    #endif
	
	vec3 vcol = pow( vec3(scol), vec3(0.9,1.1,1.4) );
	
	vec2 uv = fragCoord.xy/iResolution.xy;
	vcol *= 0.7 + 0.3*pow(16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y),0.25);

	
	fragColor = vec4( vcol, 1.0 );
}
)";
