#include "Header.h"

__device__ float3 ACEScct_to_Lin(float3 in) 
{

    float r = in.x > 0.155251141552511f ? exp2f(in.x * 17.52f - 9.72f) : (in.x - 0.0729055341958355f) / 10.5402377416545f;
    float g = in.y > 0.155251141552511f ? exp2f(in.y * 17.52f - 9.72f) : (in.y - 0.0729055341958355f) / 10.5402377416545f;
    float b = in.z > 0.155251141552511f ? exp2f(in.z * 17.52f - 9.72f) : (in.z - 0.0729055341958355f) / 10.5402377416545f;

    return make_float3(r, g, b);

}

__device__ float3 ACEScc_to_Lin(float3 in)
{

    float r = in.x < -0.3013698630f ? (exp2f(in.x * 17.52f - 9.72f) - exp2f(-16.0f)) * 2.0f : exp2f(in.x * 17.52f - 9.72f);
    float g = in.y < -0.3013698630f ? (exp2f(in.y * 17.52f - 9.72f) - exp2f(-16.0f)) * 2.0f : exp2f(in.y * 17.52f - 9.72f);
    float b = in.z < -0.3013698630f ? (exp2f(in.z * 17.52f - 9.72f) - exp2f(-16.0f)) * 2.0f : exp2f(in.z * 17.52f - 9.72f);
    
    return make_float3(r, g, b);
}


__device__ float3 DaVinciIntermediate_to_Lin(float3 in)
{

    float r = in.x > .00262409f ? (log2f(in.x + .0075f) + 7.f) * .07329248 : in.x * 10.44426855;
    float g = in.y > .00262409f ? (log2f(in.y + .0075f) + 7.f) * .07329248 : in.y * 10.44426855;
    float b = in.z > .00262409f ? (log2f(in.z + .0075f) + 7.f) * .07329248 : in.z * 10.44426855;

    return make_float3(r, g, b);

}

__device__ float3 LogC_to_Lin(float3 in)
{

    float r = in.x > 0.1496582f ? (powf(10.0f, (in.x - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f : (in.x - 0.092809f) / 5.367655f;
    float g = in.y > 0.1496582f ? (powf(10.0f, (in.y - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f : (in.y - 0.092809f) / 5.367655f;
    float b = in.z > 0.1496582f ? (powf(10.0f, (in.z - 0.385537f) / 0.2471896f) - 0.052272f) / 5.555556f : (in.z - 0.092809f) / 5.367655f;

    return make_float3(r, g, b);

}

__device__ float3 Log3G10_to_Lin(float3 in)
{
 
    float r = (in.x < 0.0f ? (in.x / 15.1927f) : (powf(10.0f, in.x / 0.224282f) - 1.0f) / 155.975327f) - 0.01f;
    float g = (in.y < 0.0f ? (in.y / 15.1927f) : (powf(10.0f, in.y / 0.224282f) - 1.0f) / 155.975327f) - 0.01f;
    float b = (in.z < 0.0f ? (in.z / 15.1927f) : (powf(10.0f, in.z / 0.224282f) - 1.0f) / 155.975327f) - 0.01f;
    
    return make_float3(r, g, b);
}

__device__ float3 Cineon_to_Lin(float3 in) 
{

    float r = (powf(10, (1023.f * in.x - 685.f) / 300.f) - 0.0108f) / (1 - 0.0108f);
    float g = (powf(10, (1023.f * in.y - 685.f) / 300.f) - 0.0108f) / (1 - 0.0108f);
    float b = (powf(10, (1023.f * in.z - 685.f) / 300.f) - 0.0108f) / (1 - 0.0108f);

    return make_float3(r, g, b); 

}

__device__ float3 Gamma24_to_Lin(float3 in) 
{

    float r = powf(in.x, 2.4f);
    float g = powf(in.y, 2.4f);
    float b = powf(in.z, 2.4f);

    return make_float3(r, g, b); 

}


__device__ float3 Lin_to_ACEScct(float3 in)
{

    float r = in.x <= 0.0078125f ? 10.5402377416545f * in.x + 0.0729055341958355f : (log2f(in.x) + 9.72f) / 17.52f;
    float g = in.y <= 0.0078125f ? 10.5402377416545f * in.y + 0.0729055341958355f : (log2f(in.y) + 9.72f) / 17.52f;
    float b = in.z <= 0.0078125f ? 10.5402377416545f * in.z + 0.0729055341958355f : (log2f(in.z) + 9.72f) / 17.52f;

    return make_float3(r, g, b);

}

__device__ float3 Lin_to_ACEScc(float3 in)
{

    float r = in.x <= 0.0f ?  -0.3584474886f : in.x < exp2f(-15.0f) ? (log2f( exp2f(-16.0f) + in.x * 0.5f) + 9.72f) / 17.52f : (log2f(in.x) + 9.72f) / 17.52f;
    float g = in.y <= 0.0f ?  -0.3584474886f : in.y < exp2f(-15.0f) ? (log2f( exp2f(-16.0f) + in.y * 0.5f) + 9.72f) / 17.52f : (log2f(in.y) + 9.72f) / 17.52f;
    float b = in.z <= 0.0f ?  -0.3584474886f : in.z < exp2f(-15.0f) ? (log2f( exp2f(-16.0f) + in.z * 0.5f) + 9.72f) / 17.52f : (log2f(in.z) + 9.72f) / 17.52f;

    return make_float3(r, g, b);

}

__device__ float3 Lin_to_DaVinciIntermediate(float3 in)
{

    float r = in.x > .02740668 ? powf(2.f, (in.x / .07329248f) - 7.f) - .0075f : in.x / 10.44426855;
    float g = in.y > .02740668 ? powf(2.f, (in.y / .07329248f) - 7.f) - .0075f : in.y / 10.44426855;
    float b = in.z > .02740668 ? powf(2.f, (in.z / .07329248f) - 7.f) - .0075f : in.z / 10.44426855;

    return make_float3(r, g, b);

}

__device__ float3 Lin_to_LogC(float3 in)
{

    float r = in.x > 0.010591f ? 0.24719f * log10(5.555556f * in.x + 0.052272f) + 0.385537f : 5.367655f * in.x + 0.092809f;
    float g = in.y > 0.010591f ? 0.24719f * log10(5.555556f * in.y + 0.052272f) + 0.385537f : 5.367655f * in.y + 0.092809f;
    float b = in.z > 0.010591f ? 0.24719f * log10(5.555556f * in.z + 0.052272f) + 0.385537f : 5.367655f * in.z + 0.092809f;

    return make_float3(r, g, b);

}


__device__ float3 Lin_to_Log3G10(float3 in)
{

    in.x += 0.01f;
    in.y += 0.01f;
    in.z += 0.01f;

    float r = in.x < 0.0f ? in.x * 15.1927f : 0.224282f * log10f((in.x * 155.975327f) + 1.0f);
    float g = in.y < 0.0f ? in.y * 15.1927f : 0.224282f * log10f((in.y * 155.975327f) + 1.0f);
    float b = in.z < 0.0f ? in.z * 15.1927f : 0.224282f * log10f((in.z * 155.975327f) + 1.0f);

    return make_float3(r, g, b);

}


__device__ float3 Lin_to_Cineon(float3 in)
{

    float r = ((log10f(in.x * (1.f - .0108f) + 0.0108f) * 300.f ) + 685.f) / 1023.f;
    float g = ((log10f(in.y * (1.f - .0108f) + 0.0108f) * 300.f ) + 685.f) / 1023.f;
    float b = ((log10f(in.z * (1.f - .0108f) + 0.0108f) * 300.f ) + 685.f) / 1023.f;

    return make_float3(r, g, b);

}


__device__ float3 Lin_to_Gamma24(float3 in)
{

    float r = powf(in.x, (1 / 2.4f));
    float g = powf(in.y, (1 / 2.4f));
    float b = powf(in.z, (1 / 2.4f));

    return make_float3(r, g, b);

}

__device__ float3 apply_input_tx(float3 in,int colorspace)
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

__device__ float3 apply_output_tx(float3 in,int colorspace)
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

__device__ float adjust_points(float channel_val, float point_val)
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
__global__ void PointsAdjustKernel(bool bActive, int p_Width, int p_Height, double globalPoints, double redPoints, double greenPoints, double bluePoints, int style, int colorspace, bool exposure,  const float* p_Input, float* p_Output)
{
   const int x = blockIdx.x * blockDim.x + threadIdx.x;
   const int y = blockIdx.y * blockDim.y + threadIdx.y;

   if ((x < p_Width) && (y < p_Height))
   {
       const int index = ((y * p_Width) + x) * 4;


	   float3 in={ p_Input[index + 0], p_Input[index + 1], p_Input[index + 2]};

	   if(bActive==false)
	   {
		   p_Output[index + 0] = in.x;
		   p_Output[index + 1] = in.y;
		   p_Output[index + 2] = in.z;
		   p_Output[index + 3] = p_Input[index + 3];
			return;
	   }

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

	if (exposure)
	{
		float post_exposure = (in.x + in.y + in.z) / 3.f;
		float exposure_neutralize = pre_exposure / post_exposure;
		in.x *= exposure_neutralize;
		in.y *= exposure_neutralize;
		in.z *= exposure_neutralize;
	}
  
	 float3 out = apply_output_tx(in, colorspace);
       p_Output[index + 0] = out.x;
       p_Output[index + 1] = out.y;
       p_Output[index + 2] = out.z;
       p_Output[index + 3] = p_Input[index + 3];
   }
}

void RunCudaKernel(void* p_Stream, int p_Width, int p_Height, double globalPoints, double redPoints, double greenPoints, double bluePoints,int style, int colorspace,bool exposure, bool bActive, const float* p_Input, float* p_Output)
{
    dim3 threads(128, 1, 1);
    dim3 blocks(((p_Width + threads.x - 1) / threads.x), p_Height, 1);
    cudaStream_t stream = static_cast<cudaStream_t>(p_Stream);

    PointsAdjustKernel<<<blocks, threads, 0, stream>>>( bActive, p_Width, p_Height, globalPoints, redPoints, greenPoints, bluePoints, style, colorspace, exposure,  p_Input, p_Output);
}
