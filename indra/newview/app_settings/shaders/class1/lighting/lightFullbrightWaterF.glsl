/** 
 * @file lightFullbrightWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

 


uniform sampler2D diffuseMap;

void fullbright_lighting_water()
{
	gl_FragColor = texture2D(diffuseMap, gl_TexCoord[0].xy);
}

