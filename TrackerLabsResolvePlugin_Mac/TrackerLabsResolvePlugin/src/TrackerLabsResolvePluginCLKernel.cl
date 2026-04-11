

#define CS_ACESCCT 0
#define CS_ACESCC 1
#define CS_DAVINCIINTERMEDIATE 2
#define CS_LOGC 3
#define CS_LOG3G10 4
#define CS_FILMSCAN 5

#define CS_LINEAR 6
#define CS_GAMMA 7

#define AS_NEGATIVE 0
#define AS_POSITIVE 1

 float3 ACEScct_to_Lin(float3 in) 
{

    float r = in.x > 0.155251141552511f ? exp2(in.x * 17.52f - 9.72f) : (in.x - 0.0729055341958355f) / 10.5402377416545f;
    float g = in.y > 0.155251141552511f ? exp2(in.y * 17.52f - 9.72f) : (in.y - 0.0729055341958355f) / 10.5402377416545f;
    float b = in.z > 0.155251141552511f ? exp2(in.z * 17.52f - 9.72f) : (in.z - 0.0729055341958355f) / 10.5402377416545f;

    return (float3)(r, g, b);

}

 float3 ACEScc_to_Lin(float3 in)
{

    float r = in.x < -0.3013698630f ? (exp2(in.x * 17.52f - 9.72f) - exp2(-16.0f)) * 2.0f : exp2(in.x * 17.52f - 9.72f);
    float g = in.y < -0.3013698630f ? (exp2(in.y * 17.52f - 9.72f) - exp2(-16.0f)) * 2.0f : exp2(in.y * 17.52f - 9.72f);
    float b = in.z < -0.3013698630f ? (exp2(in.z * 17.52f - 9.72f) - exp2(-16.0f)) * 2.0f : exp2(in.z * 17.52f - 9.72f);
    
    return (float3)(r, g, b);
}


 float3 DaVinciIntermediate_to_Lin(float3 in)
{

    float r = in.x > .00262409f ? (log2(in.x + .0075f) + 7.f) * .07329248 : in.x * 10.44426855;
    float g = in.y > .00262409f ? (log2(in.y + .0075f) + 7.f) * .07329248 : in.y * 10.44426855;
    float b = in.z > .00262409f ? (log2(in.z + .0075f) + 7.f) * .07329248 : in.z * 10.44426855;

    return (float3)(r, g, b);

}

 float3 LogC_to_Lin(float3 in)
{

    float r = in.x > 0.1496582f ? (pow(10.0f, (in.x - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f : (in.x - 0.092809f) / 5.367655f;
    float g = in.y > 0.1496582f ? (pow(10.0f, (in.y - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f : (in.y - 0.092809f) / 5.367655f;
    float b = in.z > 0.1496582f ? (pow(10.0f, (in.z - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f : (in.z - 0.092809f) / 5.367655f;

    return (float3)(r, g, b);

}

 float3 Log3G10_to_Lin(float3 in)
{
 
    float r = (in.x < 0.0f ? (in.x / 15.1927f) : (pow(10.0f, in.x / 0.224282f) - 1.0f) / 155.975327f) - 0.01f;
    float g = (in.y < 0.0f ? (in.y / 15.1927f) : (pow(10.0f, in.y / 0.224282f) - 1.0f) / 155.975327f) - 0.01f;
    float b = (in.z < 0.0f ? (in.z / 15.1927f) : (pow(10.0f, in.z / 0.224282f) - 1.0f) / 155.975327f) - 0.01f;
    
    return (float3)(r, g, b);
}

 float3 Cineon_to_Lin(float3 in) 
{

    float r = (pow(10, (1023.f * in.x - 685.f) / 300.f) - 0.0108f) / (1 - 0.0108f);
    float g = (pow(10, (1023.f * in.y - 685.f) / 300.f) - 0.0108f) / (1 - 0.0108f);
    float b = (pow(10, (1023.f * in.z - 685.f) / 300.f) - 0.0108f) / (1 - 0.0108f);

    return (float3)(r, g, b); 

}

 float3 Gamma24_to_Lin(float3 in) 
{

    float r = pow(in.x, 2.4f);
    float g = pow(in.y, 2.4f);
    float b = pow(in.z, 2.4f);

    return (float3)(r, g, b); 

}


 float3 Lin_to_ACEScct(float3 in)
{

    float r = in.x <= 0.0078125f ? 10.5402377416545f * in.x + 0.0729055341958355f : (log2(in.x) + 9.72f) / 17.52f;
    float g = in.y <= 0.0078125f ? 10.5402377416545f * in.y + 0.0729055341958355f : (log2(in.y) + 9.72f) / 17.52f;
    float b = in.z <= 0.0078125f ? 10.5402377416545f * in.z + 0.0729055341958355f : (log2(in.z) + 9.72f) / 17.52f;

    return (float3)(r, g, b);

}

 float3 Lin_to_ACEScc(float3 in)
{

    float r = in.x <= 0.0f ?  -0.3584474886f : in.x < exp2(-15.0f) ? (log2( exp2(-16.0f) + in.x * 0.5f) + 9.72f) / 17.52f : (log2(in.x) + 9.72f) / 17.52f;
    float g = in.y <= 0.0f ?  -0.3584474886f : in.y < exp2(-15.0f) ? (log2( exp2(-16.0f) + in.y * 0.5f) + 9.72f) / 17.52f : (log2(in.y) + 9.72f) / 17.52f;
    float b = in.z <= 0.0f ?  -0.3584474886f : in.z < exp2(-15.0f) ? (log2( exp2(-16.0f) + in.z * 0.5f) + 9.72f) / 17.52f : (log2(in.z) + 9.72f) / 17.52f;

    return (float3)(r, g, b);

}

 float3 Lin_to_DaVinciIntermediate(float3 in)
{

    float r = in.x > .02740668 ? pow(2.f, (in.x / .07329248f) - 7.f) - .0075f : in.x / 10.44426855;
    float g = in.y > .02740668 ? pow(2.f, (in.y / .07329248f) - 7.f) - .0075f : in.y / 10.44426855;
    float b = in.z > .02740668 ? pow(2.f, (in.z / .07329248f) - 7.f) - .0075f : in.z / 10.44426855;

    return (float3)(r, g, b);

}

 float3 Lin_to_LogC(float3 in)
{

    float r = in.x > 0.010591f ? 0.24719f * log10(5.555556f * in.x + 0.052272f) + 0.385537f : 5.367655f * in.x + 0.092809f;
    float g = in.y > 0.010591f ? 0.24719f * log10(5.555556f * in.y + 0.052272f) + 0.385537f : 5.367655f * in.y + 0.092809f;
    float b = in.z > 0.010591f ? 0.24719f * log10(5.555556f * in.z + 0.052272f) + 0.385537f : 5.367655f * in.z + 0.092809f;

    return (float3)(r, g, b);

}


 float3 Lin_to_Log3G10(float3 in)
{

    in.x += 0.01f;
    in.y += 0.01f;
    in.z += 0.01f;

    float r = in.x < 0.0f ? in.x * 15.1927f : 0.224282f * log10((in.x * 155.975327f) + 1.0f);
    float g = in.y < 0.0f ? in.y * 15.1927f : 0.224282f * log10((in.y * 155.975327f) + 1.0f);
    float b = in.z < 0.0f ? in.z * 15.1927f : 0.224282f * log10((in.z * 155.975327f) + 1.0f);

    return (float3)(r, g, b);

}


 float3 Lin_to_Cineon(float3 in)
{

    float r = ((log10(in.x * (1.f - .0108f) + 0.0108f) * 300.f ) + 685.f) / 1023.f;
    float g = ((log10(in.y * (1.f - .0108f) + 0.0108f) * 300.f ) + 685.f) / 1023.f;
    float b = ((log10(in.z * (1.f - .0108f) + 0.0108f) * 300.f ) + 685.f) / 1023.f;

    return (float3)(r, g, b);

}


 float3 Lin_to_Gamma24(float3 in)
{

    float r = pow(in.x, (1 / 2.4f));
    float g = pow(in.y, (1 / 2.4f));
    float b = pow(in.z, (1 / 2.4f));

    return (float3)(r, g, b);

}
float3 apply_input_tx(float3 in,int colorspace)
{
	float3 out;

	switch(colorspace)
	{
		case CS_ACESCCT:
			out = ACEScct_to_Lin(in);
		break;
		case CS_ACESCC:
			out = ACEScc_to_Lin(in);
		break;
		case CS_DAVINCIINTERMEDIATE:
			out = DaVinciIntermediate_to_Lin(in);
		break;
		case CS_LOGC:
			out = LogC_to_Lin(in);
		break;
		case CS_LOG3G10:
			 out = Log3G10_to_Lin(in);
		break;
		case CS_FILMSCAN:
			out = Cineon_to_Lin(in);
		break;
		case CS_GAMMA:
			out = Gamma24_to_Lin(in);
		break;
		case CS_LINEAR:
			out = in;
		break;

	}
	return out;
}
float3 apply_output_tx(float3 in,int colorspace)
{
	float3 out;
	switch(colorspace)
	{
		case CS_ACESCCT:
			out = Lin_to_ACEScct(in);
		break;
		case CS_ACESCC:
			out = Lin_to_ACEScc(in);
		break;
		case CS_DAVINCIINTERMEDIATE:
			out = Lin_to_DaVinciIntermediate(in);
		break;
		case CS_LOGC:
			out = Lin_to_LogC(in);
		break;
		case CS_LOG3G10:
			 out = Lin_to_Log3G10(in);
		break;
		case CS_FILMSCAN:
			out = Lin_to_Cineon(in);
		break;
		case CS_GAMMA:
			out = Lin_to_Gamma24(in);
		break;
		case CS_LINEAR:
			out = in;
		break;

	}
	return out;	
}
float adjust_points(float channel_val, float point_val)
{

	point_val = point_val / 6.5f;
	
	float remainder = fabs(fmod(point_val, 1.0f));

	if (point_val < 0)
	{

		point_val = point_val + remainder;

		for (int a = -1; a >= point_val; a--)
		{
			channel_val *= .5f;
		}


		if (remainder != 0.f)
		{
			channel_val = channel_val / (2 - (1 - remainder));	
		}
	}


	if (point_val > 0)
	{
		point_val = point_val - remainder;

		for (int a = 0; a < point_val; a++)
		{
			channel_val *= 2.f;
		}


		if (remainder != 0.f)
		{
			channel_val *= (1 + remainder);
		} 
	}
	return channel_val;
}

__kernel void PointsAdjustKernel(int width, int height, double globalPoints, double redPoints, double greenPoints, double bluePoints, int style, int colorspace,int exposure,__global float* pInput,__global float* pOutput)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	if (x >= width || y >= height) return;

	int id = (y*width + x) * 4;

	float3 in={ pInput[id + 0], pInput[id + 1], pInput[id + 2]};
	in = apply_input_tx(in,colorspace);
	globalPoints = globalPoints - 25.f;

	if (style == AS_NEGATIVE)
	{
	  globalPoints *= -1;
	}

	in.x = adjust_points(in.x, globalPoints);

    in.y = adjust_points(in.y, globalPoints);

    in.z = adjust_points(in.z, globalPoints);

    float pre_exposure = (in.x + in.y + in.z) / 3.f;

    redPoints = redPoints - 25.f;
	greenPoints = greenPoints - 25.f;
	bluePoints = bluePoints - 25.f;

	 if (style == AS_NEGATIVE)
	 {
		redPoints *= -1;
        greenPoints *= -1;
        bluePoints *= -1;
	 }

	in.x = adjust_points(in.x, redPoints);

	in.y = adjust_points(in.y, greenPoints);

	in.z = adjust_points(in.z, bluePoints);

	if (exposure == 1)
	{
		float post_exposure = (in.x + in.y + in.z) / 3.f;
		float exposure_neutralize = pre_exposure / post_exposure;
		in.x *= exposure_neutralize;
		in.y *= exposure_neutralize;
		in.z *= exposure_neutralize;
	}
  
	 float3 out = apply_output_tx(in, colorspace);
     pOutput[id + 0] = out.x;
     pOutput[id + 1] = out.y;
     pOutput[id + 2] = out.z;
     pOutput[id + 3] = pInput[id + 3];
}