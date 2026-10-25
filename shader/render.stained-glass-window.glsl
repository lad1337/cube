// https://www.shadertoy.com/view/7ldGD2
// based on Xmas Fractal 2Tweets by mplanck https://www.shadertoy.com/view/Xdj3Wh

void imageSample( out vec4 f, in vec2 w )
{
	float t = .001 * iTime + .3;
	vec4 p = abs(4.-8.*w.xyxx / iResolution.x), c=p*.0;
	p.yx *= 60. * t * 0.3;
    t *= 0.9; // not just affecting time
	for (float d=.2;d<3.;d+=.3) {
		p -= .5*d;
		for (int i=0;i<8;i++) p.xy=mat2(
            cos(t),sin(t),-sin(t),cos(t))*(p.xy+sign(p.yx)*vec2(-.2,1.6)
        );
        //p = mod(p+c, 10.);
		c += .1*p;
        c = mod(c, 4.);
        //c.xz = c.zx;
        c *= w.y/1000.; // gradient
	}
	
	f = c;
}

vec4 render(vec2 w)
{
    // I'm doing some sort of naive edge detection,
    // in order to make the border of the imagined stained glass panes.
    vec4 s1, s2, s3, s4;
    float d = 1.2;
	imageSample(s1, w+vec2(-d, 0.));
	imageSample(s2, w+vec2(+d, 0.));
	imageSample(s3, w+vec2(0., -d));
	imageSample(s4, w+vec2(0., +d));
    
    // color components -> single value (not weighted perceptually or anything)
    s1.xyzw = vec4(s1.x + s1.y + s1.z);
    s2.xyzw = vec4(s2.x + s2.y + s2.z);
    
    // take main sample
	imageSample(f, w);
    
    // apply black borders
    f -= abs(s1 - s2) * 20.;
    f -= abs(s3 - s3) * 20.;
    return f;
    
    //f -= 0.2;
    //f = pow(f, vec4(0.2));
}
