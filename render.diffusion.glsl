// Fork of " expansive reaction-diffusion" by None. https://shadertoy.com/view/-1
// 2020-11-19 16:46:22

void render(vec2 p) {
    vec2 uv = p;

    vec2 pixelSize = 1. / p.xy;
    vec2 aspect = vec2(1., 1.);

    vec4 noise = texture(iChannel3, fragCoord.xy / iChannelResolution[3].xy + fract(vec2(42, 56) * iTime));

    vec2 lightSize = vec2(4.);

    // get the gradients from the blurred image
    vec2 d = pixelSize * 2.;
    vec4 dx = (texture(iChannel2, uv + vec2(1, 0) * d) - texture(iChannel2, uv - vec2(1, 0) * d)) * 0.45;
    vec4 dy = (texture(iChannel2, uv + vec2(0, 1) * d) - texture(iChannel2, uv - vec2(0, 1) * d)) * 0.5;

    // add the pixel gradients
    d = pixelSize * 1.;
    dx += texture(iChannel0, uv + vec2(1, 0) * d) - texture(iChannel0, uv - vec2(1, 0) * d);
    dy += texture(iChannel0, uv + vec2(0, 1) * d) - texture(iChannel0, uv - vec2(0, 1) * d);

    vec2 displacement = vec2(dx.x, dy.x) * lightSize;  // using only the red gradient as displacement vector
    float light = pow(max(1. - distance(0.5 + (uv - 0.5) * aspect * lightSize + displacement,
                                        0.5 + (iMouse.xy * pixelSize - 0.5) * aspect * lightSize),
                          0.),
                      4.);

    // recolor the red channel
    vec4 rd = vec4(texture(iChannel0, uv + vec2(dx.x, dy.x) * pixelSize * 78.).x) * vec4(0.7, 1.5, 2.0, 1.0) -
              vec4(0.3, 1.0, 1.0, 1.0);

    // and add the light map
    vec4 fragColor = mix(rd, vec4(58.0, 6., 62., 61.),
                         light * 5.75 * vec4(.85 - texture(iChannel0, uv + vec2(dx.x, dy.x) * pixelSize * 68.).x));
    fragColor.r = fragColor.r * sin(iTime * 0.2);
    fragColor.g = fragColor.g * sin(iTime * 0.275342);
    fragColor.b = fragColor.b * sin(iTime * 0.5275342);
    return fragColor;
    // fragColor = texture(iChannel0, uv); // bypass
}
