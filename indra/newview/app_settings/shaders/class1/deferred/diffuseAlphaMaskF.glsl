/** 
 * @file diffuseF.glsl
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
 

uniform float minimum_alpha;
uniform float maximum_alpha;

uniform sampler2D diffuseMap;

varying vec3 vary_normal;

void main() 
{
	vec4 col = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy) * gl_Color;
	
	if (col.a < minimum_alpha || col.a > maximum_alpha)
	{
		discard;
	}

	gl_FragData[0] = vec4(col.rgb, 0.0);
	gl_FragData[1] = vec4(0,0,0,0); // spec
	vec3 nvn = normalize(vary_normal);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}
