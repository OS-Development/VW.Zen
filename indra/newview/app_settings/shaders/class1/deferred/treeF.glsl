/** 
 * @file treeF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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
out vec4 gl_FragData[3];
#endif

uniform sampler2D diffuseMap;

VARYING vec4 vertex_color;
VARYING vec3 vary_normal;
VARYING vec2 vary_texcoord0;

uniform float minimum_alpha;

void main() 
{
	vec4 col = texture2D(diffuseMap, vary_texcoord0.xy);
	if (col.a < minimum_alpha)
	{
		discard;
	}

	gl_FragData[0] = vec4(vertex_color.rgb*col.rgb, 0.0);
	gl_FragData[1] = vec4(0,0,0,0);
	vec3 nvn = normalize(vary_normal);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}
