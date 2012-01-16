/** 
 * @file shadowAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

uniform mat4 modelview_projection_matrix;

uniform float minimum_alpha;

uniform sampler2D diffuseMap;

//flat VARYING int foo;
VARYING float pos_z;
VARYING float pos_w;
VARYING float target_pos_x;
//VARYING vec4 pre_pos;
//VARYING vec4 post_pos;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

void main() 
{
	float alpha = diffuseLookup(vary_texcoord0.xy).a * vertex_color.a;

	if (alpha < 0.05)
	{
		discard;
	}

	if (alpha < 0.88)
	{
		//vec4 pos = modelview_projection_matrix * pre_pos;

		//if (0.5 > mod(vary_texcoord0.x*99999,1.0))
		//vec4 pos = post_pos;
		//		if (0.5 <= fract((253.5*(0.5+pos.x) + 0*127.5*(0.5+pos.y)))/pos.w)
		//	pos.xy *= 2.0 * 255.0 / pos.w;

		//pos.x = floor(posxw.x);
		//pos.x = floor(posxw.x / posxw.t);
		//pos.x = foo;

		//pos.x *= 1 * 253.5;
		//pos.y *= 1 * 166.0;
		//		pos.x *= 511;
		//		pos.y *= 1*330.0;
		//	pos.x = floor(pos.x);
		//	pos.y = int(pos.y);
		//		if (0.25>= fract(0.5 * (pos.x+pos.y)))
	  if (fract(0.5*floor(target_pos_x / pos_w )) < 0.25)
		  //if (0.5 >= fract((0*253.5*(0.5+pos.x) + 1*166*(0.0+pos.y)))/pos.w)
		{
			discard;
		}
	}

	gl_FragColor = vec4(1,1,1,1);
	
	gl_FragDepth = max(pos_z/pos_w*0.5+0.5, 0.0);
}
