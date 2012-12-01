cbuffer Lighting : register(cb0)
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularcolor;
	float specularPower;
	float3 lightDirection;
}

cbuffer Mapping : register(cb1)
{
	float textureflag;
	float alphaflag;
	float normalflag;
	float particleflag;
}

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
	float3 normal : NORMAL;
	float3 viewDirection : VIEWDIRECTION;
	float2 texcord : TEXCORD;
	float3x3 tbnmatrix : TBNMATRIX;
};

Texture2D Texture;
Texture2D Alpha;
Texture2D NormalMap;
SamplerState ss;

float4 PShader(VOut input) : SV_TARGET
{
	float4 textureColor;
    float4 alphaColor;
	float4 normalColor;
    float3 lightDir;
    float lightIntensity;
    float4 color;
    float3 reflection;
    float4 specular;
	float resAlpha;

    // Sample the pixel color from the texture using the sampler at this texture coordinate location.
	if(alphaflag == 1.)
	{
		alphaColor = Alpha.Sample(ss, input.texcord);
	}
	if(textureflag == 1.)
	{
		textureColor = Texture.Sample(ss, input.texcord);
		if(alphaflag == 1.)
		{
			textureColor.a = (alphaColor.r + alphaColor.g + alphaColor.b ) / 3.0;
			if(textureColor.a < 0.1f)
			{
				textureColor.a = 0.0f;
			}	
		}
	}
	color = ambientColor;
	if(normalflag == 1.)
	{
		normalColor = NormalMap.Sample(ss, input.texcord);
		normalColor = 2.0f*normalColor-1.0f;
		input.normal = normalize( float3(normalColor.x,normalColor.y,normalColor.z));
		input.normal = mul(input.normal, transpose(input.tbnmatrix));
		//lightDir = mul(lightDirection,input.tbnmatrix);
	}

    // Set the default output color to the ambient light value for all pixels.
    

  
    // Invert the light direction for calculations.
    lightDir = -lightDirection;

	lightDir = normalize(lightDir);
    // Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(input.normal, lightDir));
  
	//Initialize the specular color.
    specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if(lightIntensity > 0.0f)
    {
        // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
        color += (diffuseColor * lightIntensity);

        // Saturate the ambient and diffuse color.
        color = saturate(color);
		if(particleflag == 0)
		{
			// Calculate the reflection vector based on the light intensity, normal vector, and light direction.
			reflection = normalize(2 * lightIntensity * input.normal - lightDir); 
		
			// Determine the amount of specular light based on the reflection vector, viewing direction, and specular power.
			specular = pow(saturate(dot(reflection, input.viewDirection)), specularPower);
		}
    }

    // Multiply the texture pixel and the input color to get the textured result.

	if(textureflag != 1.)
	{
		color = color * input.color;
	}
	else
	{
		color = color * textureColor;
	}

	if(particleflag == 0)
	{  
		// Add the specular component last to the output color.
		color = saturate(color + specular);
		//color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if(alphaflag == 1.)
	{
		color.a = (alphaColor.r + alphaColor.g + alphaColor.b ) / 3.0;
		if(color.a < 0.1f)
		{
			color.a = 0.0f;
		}	
	}

    return color;
}
