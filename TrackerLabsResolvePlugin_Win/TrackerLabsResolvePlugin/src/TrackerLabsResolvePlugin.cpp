#include "TrackerLabsResolvePlugin.h"

#include <stdio.h>

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"
#include "ofxsLog.h"
#include "Rijndael.h"
#include "Header.h"
#include "activation.h"
#include <regex>
#ifndef __APPLE__
#include <CL/cl.h>
#endif
#include <math.h>
////////////////////////////////////////////////////////////////////////////////

class ImageScaler : public OFX::ImageProcessor
{
public:
    explicit ImageScaler(OFX::ImageEffect& p_Instance);

    virtual void processImagesCUDA();
    virtual void processImagesOpenCL();
    virtual void processImagesMetal();
    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);

	void setThumbIcon(bool thumb) { _thumb = thumb; }
    void setSrcImg(OFX::Image* p_SrcImg);
	void setParams(double gloalPoints, double redPoints, double greenPoints, double bluePoints,int style, int colorspace,bool bexposure);
	void setOpenCLProgram(char* openclProgram){ _openclProgram = openclProgram; }
	void setActiveStatus(int status){ _activeStatus = status; }
	void processing(float* buf, int width, int height);
	bool isRedXPixel(int x, int y, int width, int height);
	void PlotPixel(float* buf, int width, int height, int x, int y, Color3 color);
	void PlotPixelAlpha(float* buf, int width, int height, int x, int y, Color3 color, int alpha);
	void DrawLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color);
	void DrawLineWithThickness(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int thickness);
	void DrawDottedLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int dashLen, int gapLen);
	void DrawDottedLineWithStep(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int dashLen, int gapLen, int& step);
	void DrawCurvedLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, float curvature01, int curveSign);
	void DrawCurvedDottedLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int dashLen, int gapLen, float curvature01, int curveSign);
	void DrawFilledCircle(float* buf, int width, int height, int cx, int cy, int radius, Color3 color, int alpha = 255);
	void DrawCircleOutline(float* buf, int width, int height, int cx, int cy, int radius, Color3 color);
	void DrawFilledBox(float* buf, int width, int height, int cx, int cy, int halfW, int halfH, Color3 color, int alpha);
	void InvertBoxRegion(float* buf, int width, int height, int cx, int cy, int halfW, int halfH);
	void DrawBox(float* buf, int width, int height, int cx, int cy, int halfW, int halfH, Color3 color);
	void DrawLabel(float* imgbuf, int width, int height, int id, int index, float time, int x, int y, Color3 color, int scale, int labelType);
	void DrawChar(float* buf, int width, int height, char c, int x, int y, Color3 color, int scale);
	void DrawKanji(float* buf, int width, int height, const unsigned char* glyph, int x, int y, Color3 color, int scale);
	void DrawHexLabel(float* buf, int width, int height, int id, int x, int y, Color3 color, int scale);

private:
	bool _thumb;
    OFX::Image* _srcImg;
	double _globalPoints;
	double _redPoints;
	double _greenPoints;
	double _bluePoints;
	int _style;
	int _colorspace;
	bool _exposure;
	char* _openclProgram;
	bool _activeStatus;
public:
	float _time;
	Color3 _tgt;
	float _tolerance;
	int _maxObjects;
	float _boxSize;
	Color3 _lineClr;
	float _lineWidth;
	float _lineCurvature;
	float _fadeByDistance;
	float _nodeOutlineWidth;
	Color3 _nodeClr;
	Color3 _textClr;
	bool _triToneEnabled;
	Color3 _triClr2;
	Color3 _triClr3;
	float _speedPercent;
	bool _linesEnabled;
	int _connectType;
	float _maxDist;
	int _maxConnections;
	float _connectChance;

	int _nodeShape;
	float _fillOpacity;
	int _fillAlpha;
	int _textScale;
	bool _showLabels;
	int _labelType;
	float _invertFillPercent;
	float _labelDensity;

};

ImageScaler::ImageScaler(OFX::ImageEffect& p_Instance)
    : OFX::ImageProcessor(p_Instance)
{
}

#ifndef __APPLE__
extern void RunCudaKernel(void* p_Stream, int p_Width, int p_Height, double globalPoints, double redPoints, double greenPoints, double bluePoints, int style, int colorspace, bool exposure, bool bActive, const float* p_Input, float* p_Output);
#endif

void ImageScaler::processImagesCUDA()
{
#ifndef __APPLE__
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());
	//RunCudaKernel(_pCudaStream, width, height, _globalPoints, _redPoints, _greenPoints, _bluePoints, _style, _colorspace, _exposure, bActive,input, output);
#endif
}

#ifdef __APPLE__
extern void CopyBufferFromGPUToCPU(const float* pGPU, float* pCPU, int size);
extern void CopyBufferFromCPUToGPU(const float* pGPU, float* pCPU, int size);
#endif

void ImageScaler::processImagesMetal()
{
#ifdef __APPLE__
    
        const OfxRectI& bounds = _srcImg->getBounds();
       const int width = bounds.x2 - bounds.x1;
       const int height = bounds.y2 - bounds.y1;

       float* input = static_cast<float*>(_srcImg->getPixelData());
       float* output = static_cast<float*>(_dstImg->getPixelData());

       int buf_size = 4 * sizeof(float) * width * height;
       float* buf = new float[buf_size];
       memset(buf, 0, buf_size);

       CopyBufferFromGPUToCPU(input, buf, buf_size);
       if (!_thumb)
       {
           processing(buf, width, height);
       }
       
       CopyBufferFromCPUToGPU(output, buf, buf_size);
       free(buf);
    
#endif
}

extern void RunOpenCLKernel(void* p_CmdQ, char* openclProgram, int p_Width, int p_Height, double globalPoints, double redPoints, double greenPoints, double bluePoints, int style, int colorspace, int exposure, bool bActive, const float* p_Input, float* p_Output);

void ImageScaler::processImagesOpenCL()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

#ifndef __APPLE__
	cl_command_queue cmdQ = static_cast<cl_command_queue>(_pOpenCLCmdQ);
	if(_thumb)
		clEnqueueCopyBuffer(cmdQ, (cl_mem)input, (cl_mem)output, 0, 0, 4 * sizeof(float) * width * height, 0, NULL, NULL);
	else
	{
		int buf_size = 4 * sizeof(float) * width * height;
		float* buf = new float[buf_size];
		memset(buf, 0, buf_size);
		clEnqueueReadBuffer(cmdQ, (cl_mem)input, CL_TRUE, 0, buf_size, buf, 0, NULL, NULL);
		
		processing(buf, width, height);

		clEnqueueWriteBuffer(cmdQ, (cl_mem)output, CL_TRUE, 0, buf_size, buf, 0, NULL, NULL);

		free(buf);
	}
#endif
}
void ImageScaler::PlotPixel(float* buf, int width, int height, int x, int y, Color3 color)
{
	if (x < 0 || x >= width || y < 0 || y >= height) return;
	int index = y * width + x;
	buf[4 * index] = color.r;
	buf[4 * index+1] = color.g;
	buf[4 * index+2] = color.b;
}
void ImageScaler::PlotPixelAlpha(float* buf, int width, int height, int x, int y, Color3 color, int alpha)
{
	if (x < 0 || x >= width || y < 0 || y >= height) return;
	float a = alpha / 255.0f;
	float inv = 1.0f - a;

	int index = y * width + x;
	buf[4 * index] = color.r*a+inv* buf[4 * index];
	buf[4 * index + 1] = color.g * a + inv* buf[4 * index + 1];
	buf[4 * index + 2] = color.b * a + inv* buf[4 * index + 2];

}
void ImageScaler::DrawLineWithThickness(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int thickness)
{
	if (thickness <= 0) return;

	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;

	int half = thickness / 2;
	int start = -half;
	int end = thickness - half - 1;

	while (1) {
		// Thicken the line by plotting a small square around each Bresenham step.
		for (int oy = start; oy <= end; oy++) {
			for (int ox = start; ox <= end; ox++) {
				PlotPixel(buf, width, height, x0 + ox, y0 + oy, color);
			}
		}
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}
void ImageScaler::DrawLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color)
{
	int thickness = (int)(_lineWidth + 0.5f);
	DrawLineWithThickness(buf, width, height, x0, y0, x1, y1, color, thickness);
}
void ImageScaler::DrawDottedLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int dashLen, int gapLen)
{
	int step = 0;
	DrawDottedLineWithStep(buf, width, height, x0, y0, x1, y1, color, dashLen, gapLen, step);
}
void ImageScaler::DrawDottedLineWithStep(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int dashLen, int gapLen, int& step)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;
	int thickness = (int)(_lineWidth + 0.5f);
	if (thickness <= 0) return;
	int half = thickness / 2;
	int start = -half;
	int end = thickness - half - 1;

	int cycleLen = dashLen + gapLen;
	if (cycleLen <= 0) cycleLen = 1; // Prevent modulo by zero.

	while (1) {
		// Draw only during dash portion (keep `step` continuous across segments).
		if ((step % cycleLen) < dashLen) {
			for (int oy = start; oy <= end; oy++) {
				for (int ox = start; ox <= end; ox++) {
					PlotPixel(buf, width, height, x0 + ox, y0 + oy, color);
				}
			}
		}
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
		step++;
	}
}

void ImageScaler::DrawCurvedLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, float curvature01, int curveSign)
{
	if (curvature01 <= 0.0f) {
		DrawLine(buf, width, height, x0, y0, x1, y1, color);
		return;
	}

	float dx = (float)(x1 - x0);
	float dy = (float)(y1 - y0);
	float dist = sqrtf(dx * dx + dy * dy);
	if (dist < 0.001f) {
		DrawLine(buf, width, height, x0, y0, x1, y1, color);
		return;
	}

	// Perpendicular unit vector.
	float nx = -dy / dist;
	float ny = dx / dist;

	// Quadratic Bezier: P0 -> P1 -> P2
	float midx = (x0 + x1) * 0.5f;
	float midy = (y0 + y1) * 0.5f;
	float mag = curvature01 * dist * 0.25f * (float)curveSign;
	float cx = midx + nx * mag;
	float cy = midy + ny * mag;

	// Sample enough segments so the curve feels smooth.
	int samples = (int)(dist / 3.0f) + 2;
	if (samples < 2) samples = 2;
	if (samples > 200) samples = 200;

	float prevx = (float)x0;
	float prevy = (float)y0;

	for (int s = 1; s <= samples; s++) {
		float t = (float)s / (float)samples;
		float omt = 1.0f - t;

		float x = omt * omt * (float)x0 + 2.0f * omt * t * cx + t * t * (float)x1;
		float y = omt * omt * (float)y0 + 2.0f * omt * t * cy + t * t * (float)y1;

		int xi0 = (int)(prevx + (prevx >= 0.0f ? 0.5f : -0.5f));
		int yi0 = (int)(prevy + (prevy >= 0.0f ? 0.5f : -0.5f));
		int xi1 = (int)(x + (x >= 0.0f ? 0.5f : -0.5f));
		int yi1 = (int)(y + (y >= 0.0f ? 0.5f : -0.5f));

		if (xi0 != xi1 || yi0 != yi1) {
			DrawLine(buf, width, height, xi0, yi0, xi1, yi1, color);
		}

		prevx = x;
		prevy = y;
	}
}

void ImageScaler::DrawCurvedDottedLine(float* buf, int width, int height, int x0, int y0, int x1, int y1, Color3 color, int dashLen, int gapLen, float curvature01, int curveSign)
{
	if (curvature01 <= 0.0f) {
		DrawDottedLine(buf, width, height, x0, y0, x1, y1, color, dashLen, gapLen);
		return;
	}

	float dx = (float)(x1 - x0);
	float dy = (float)(y1 - y0);
	float dist = sqrtf(dx * dx + dy * dy);
	if (dist < 0.001f) {
		DrawDottedLine(buf, width, height, x0, y0, x1, y1, color, dashLen, gapLen);
		return;
	}

	float nx = -dy / dist;
	float ny = dx / dist;

	float midx = (x0 + x1) * 0.5f;
	float midy = (y0 + y1) * 0.5f;
	float mag = curvature01 * dist * 0.25f * (float)curveSign;
	float cx = midx + nx * mag;
	float cy = midy + ny * mag;

	int samples = (int)(dist / 3.0f) + 2;
	if (samples < 2) samples = 2;
	if (samples > 200) samples = 200;

	int step = 0;
	float prevx = (float)x0;
	float prevy = (float)y0;

	for (int s = 1; s <= samples; s++) {
		float t = (float)s / (float)samples;
		float omt = 1.0f - t;

		float x = omt * omt * (float)x0 + 2.0f * omt * t * cx + t * t * (float)x1;
		float y = omt * omt * (float)y0 + 2.0f * omt * t * cy + t * t * (float)y1;

		int xi0 = (int)(prevx + (prevx >= 0.0f ? 0.5f : -0.5f));
		int yi0 = (int)(prevy + (prevy >= 0.0f ? 0.5f : -0.5f));
		int xi1 = (int)(x + (x >= 0.0f ? 0.5f : -0.5f));
		int yi1 = (int)(y + (y >= 0.0f ? 0.5f : -0.5f));

		if (xi0 != xi1 || yi0 != yi1) {
			DrawDottedLineWithStep(buf, width, height, xi0, yi0, xi1, yi1, color, dashLen, gapLen, step);
		}

		prevx = x;
		prevy = y;
	}
}
void ImageScaler::DrawFilledCircle(float* buf, int width, int height, int cx, int cy, int radius, Color3 color, int alpha)
{
	for (int y = -radius; y <= radius; y++) {
		for (int x = -radius; x <= radius; x++) {
			if (x * x + y * y <= radius * radius) {
				if (alpha >= 255) {
					PlotPixel(buf, width, height,cx + x, cy + y, color);
				}
				else {
					PlotPixelAlpha(buf, width, height, cx + x, cy + y, color, alpha);
				}
			}
		}
	}
}
void ImageScaler::DrawCircleOutline(float* buf, int width, int height, int cx, int cy, int radius, Color3 color)
{
	int thickness = (int)(_nodeOutlineWidth + 0.5f);
	if (thickness < 1) thickness = 1;
	//if (thickness > 10) thickness = 10; // Keep outlines reasonable.
	int half = thickness / 2;
	int x = radius, y = 0, err = 0;
	while (x >= y) {
		// Plot each outline pixel as a small filled square for "stroke" thickness.
		for (int oy = -half; oy <= half; oy++) {
			for (int ox = -half; ox <= half; ox++) {
				PlotPixel(buf, width, height, cx + x + ox, cy + y + oy, color);
				PlotPixel(buf, width, height, cx + y + ox, cy + x + oy, color);
				PlotPixel(buf, width, height, cx - y + ox, cy + x + oy, color);
				PlotPixel(buf, width, height, cx - x + ox, cy + y + oy, color);
				PlotPixel(buf, width, height, cx - x + ox, cy - y + oy, color);
				PlotPixel(buf, width, height, cx - y + ox, cy - x + oy, color);
				PlotPixel(buf, width, height, cx + y + ox, cy - x + oy, color);
				PlotPixel(buf, width, height, cx + x + ox, cy - y + oy, color);
			}
		}
		y++;
		if (err <= 0) { err += 2 * y + 1; }
		if (err > 0) { x--; err -= 2 * x + 1; }
	}
}
void ImageScaler::DrawFilledBox(float* buf, int width, int height, int cx, int cy, int halfW, int halfH, Color3 color, int alpha)
{
	int left = cx - halfW, right = cx + halfW;
	int top = cy - halfH, bottom = cy + halfH;
	for (int y = top; y <= bottom; y++) {
		for (int x = left; x <= right; x++) {
			PlotPixelAlpha(buf, width, height, x, y, color, alpha);
		}
	}
}
void ImageScaler::InvertBoxRegion(float* buf, int width, int height, int cx, int cy, int halfW, int halfH)
{
	int left = cx - halfW + 1;   // +1 to not invert the outline pixels
	int right = cx + halfW - 1;  // -1 to not invert the outline pixels
	int top = cy - halfH + 1;
	int bottom = cy + halfH - 1;

	// Clamp to world bounds
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= width) right = width - 1;
	if (bottom >= height) bottom = height - 1;

	for (int y = top; y <= bottom; y++) {
		for (int x = left; x <= right; x++) {
			int index = y * width + x;
			buf[4 * index] = 1 - buf[4 * index];
			buf[4 * index+1] = 1 - buf[4 * index+1];
			buf[4 * index+2] = 1 - buf[4 * index+2];
			// Keep alpha unchanged
		}
	}
}
void ImageScaler::DrawBox(float* buf, int width, int height, int cx, int cy, int halfW, int halfH, Color3 color)
{
	int left = cx - halfW, right = cx + halfW;
	int top = cy - halfH, bottom = cy + halfH;
	int thickness = (int)(_nodeOutlineWidth + 0.5f);
	if (thickness < 1) thickness = 1;
	//if (thickness > 10) thickness = 10; // Keep outlines reasonable.
	DrawLineWithThickness(buf, width, height, left, top, right, top, color, thickness);
	DrawLineWithThickness(buf, width, height, right, top, right, bottom, color, thickness);
	DrawLineWithThickness(buf, width, height, right, bottom, left, bottom, color, thickness);
	DrawLineWithThickness(buf, width, height, left, bottom, left, top, color, thickness);
}
void  ImageScaler::DrawChar(float* buf, int width, int height, char c, int x, int y, Color3 color, int scale)
{
	const unsigned char* glyph = NULL;
	static const unsigned char GLYPH_X[5] = { 0x5, 0x5, 0x2, 0x5, 0x5 };
	static const unsigned char GLYPH_COLON[5] = { 0x0, 0x2, 0x0, 0x2, 0x0 };

	if (c >= '0' && c <= '9') glyph = DIGITS[c - '0'];
	else if (c >= 'A' && c <= 'F') glyph = HEX_CHARS[c - 'A'];
	else if (c == 'X' || c == 'x') glyph = GLYPH_X;
	else if (c == ':') glyph = GLYPH_COLON;

	if (!glyph) return;
	for (int row = 0; row < 5; row++) {
		for (int col = 0; col < 3; col++) {
			if (glyph[4-row] & (1 << (2 -col))) {
				for (int sy = 0; sy < scale; sy++)
					for (int sx = 0; sx < scale; sx++)
						PlotPixel(buf, width, height, x + col * scale + sx, y + row * scale + sy, color);
			}
		}
	}
}
void ImageScaler::DrawKanji(float* buf, int width, int height, const unsigned char* glyph, int x, int y, Color3 color, int scale)
{
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			if (glyph[7-row] & (1 << (7 - col))) {
				for (int sy = 0; sy < scale; sy++)
					for (int sx = 0; sx < scale; sx++)
						PlotPixel(buf, width, height, x + col * scale + sx, y + row * scale + sy, color);
			}
		}
	}
}
void ImageScaler::DrawHexLabel(float* buf, int width, int height, int id, int x, int y, Color3 color, int scale)
{
	char hex[8];
	snprintf(hex, sizeof(hex), "0X%04X", id & 0xFFFF);
	int px = x;
	int charWidth = 4 * scale;
	for (int i = 0; hex[i]; i++) {
		DrawChar(buf, width, height, hex[i], px, y, color, scale);
		px += charWidth;
	}
}
void ImageScaler::DrawLabel(float* imgbuf, int width, int height, int id, int index, float time, int x, int y, Color3 color, int scale, int labelType)
{
	char buf[32];
	int px = x;
	int charWidth = 4 * scale;
	int kanjiWidth = 9 * scale;

	// Animation cycle - changes every ~2 seconds
	int animCycle = (int)(time * 0.5f) + index;

	switch (labelType) {
	case LC_COORDINATES:
	case LC_RANDOM_HEX:
	default:
		// Animated hex - shifts over time
	{
		int animatedId = id + (int)(time * 10) * (index + 1);
		DrawHexLabel(imgbuf,width,height, animatedId, x, y, color, scale);
	}
	break;

	case LC_SEQUENTIAL001:
		// TRK-001, LAB-002, etc - varies word and animates number
	{
		const char* word = SEQ_WORDS[(index + animCycle) % SEQ_WORDS_COUNT];
		int num = ((index + 1) + (int)(time * 2)) % 1000;
		snprintf(buf, sizeof(buf), "%s%03d", word, num);
		for (int i = 0; buf[i]; i++) {
			char c = buf[i];
			// Draw letters too (simple versions)
			if (c >= 'A' && c <= 'Z') {
				// Simple letter rendering - use hex chars for A-F, skip others
				if (c <= 'F') DrawChar(imgbuf, width, height, c, px, y, color, scale);
				else {
					// Draw a dot for other letters
					for (int sy = 0; sy < scale * 2; sy++)
						for (int sx = 0; sx < scale * 2; sx++)
							PlotPixel(imgbuf, width, height, px + sx, y + 2 * scale + sy, color);
				}
			}
			else {
				DrawChar(imgbuf, width, height, c, px, y, color, scale);
			}
			px += charWidth;
		}
	}
	break;

	case LC_TIMECODE:
		// Animated timecode - each node shows different time
	{
		int baseFrames = (int)(time * 24);
		int offset = index * 127 + (id % 100) * 37;  // Unique offset per node
		int frames = baseFrames + offset;
		int ff = frames % 24;
		int ss = (frames / 24) % 60;
		int mm = (frames / 24 / 60) % 60;
		int hh = (frames / 24 / 60 / 60) % 24;
		snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", hh, mm, ss, ff);
		for (int i = 0; buf[i]; i++) {
			DrawChar(imgbuf, width, height, buf[i], px, y, color, scale);
			px += charWidth;
		}
	}
	break;

	case LC_BINARY:
		// Animated binary - shifts and varies per node
	{
		int animVal = (id * 17 + index * 31 + (int)(time * 8)) & 0xFF;
		for (int i = 7; i >= 0; i--) {
			DrawChar(imgbuf, width, height, (animVal & (1 << i)) ? '1' : '0', px, y, color, scale);
			px += charWidth;
		}
	}
	break;

	case LC_HEX_MEMORY:
		// Animated memory address
	{
		unsigned int addr = (unsigned int)((id << 16) | ((index * 0x1234 + (int)(time * 100)) & 0xFFFF));
		snprintf(buf, sizeof(buf), "0X%08X", addr);
		for (int i = 0; buf[i]; i++) {
			DrawChar(imgbuf, width, height, buf[i], px, y, color, scale);
			px += charWidth;
		}
	}
	break;

	case LC_JAPANESE_TRACK:
		// Cycle through Japanese tracking words
	{
		int wordIdx = (index + animCycle) % 4;
		const KanjiWord& word = JP_TRACK_WORDS[wordIdx];
		for (int i = 0; i < word.len; i++) {
			DrawKanji(imgbuf, width, height, word.chars[i], px, y, color, scale);
			px += kanjiWidth;
		}
	}
	break;

	case LC_JAPANESE_TARGET:
		// Cycle through Japanese target words
	{
		int wordIdx = (index + animCycle) % 3;
		const KanjiWord& word = JP_TARGET_WORDS[wordIdx];
		for (int i = 0; i < word.len; i++) {
			DrawKanji(imgbuf, width, height, word.chars[i], px, y, color, scale);
			px += kanjiWidth;
		}
	}
	break;

	case LC_KATAKANA:
		// Cycle through Katakana words
	{
		int wordIdx = (index + animCycle) % 6;
		const KanjiWord& word = KATA_WORDS[wordIdx];
		for (int i = 0; i < word.len; i++) {
			DrawKanji(imgbuf, width, height, word.chars[i], px, y, color, scale);
			px += kanjiWidth;
		}
	}
	break;

	case LC_CHINESE_LOCK:
		// Cycle through Chinese words
	{
		int wordIdx = (index + animCycle) % 3;
		const KanjiWord& word = CN_LOCK_WORDS[wordIdx];
		for (int i = 0; i < word.len; i++) {
			DrawKanji(imgbuf, width, height, word.chars[i], px, y, color, scale);
			px += kanjiWidth;
		}
	}
	break;
	}
}
// --- TRACKING PERSISTENCE ---
static std::vector<Blob> g_cachedBlobs;
static int g_lastUpdateFrame = -999;
static int g_lastTargetR = -1, g_lastTargetG = -1, g_lastTargetB = -1;
bool ImageScaler::isRedXPixel(int x, int y, int width, int height)
{
	// 1. Bounds check: ensure the pixel is inside the rectangle
	if (x < 0 || x >= width || y < 0 || y >= height) return false;

	// Use long long to prevent overflow during multiplication
	long long x_h = (long long)x * height;
	long long y_w = (long long)y * width;
	long long inv_y_w = (long long)(height - 1 - y) * width;

	// Use a tolerance for thickness (e.g., width / 2) 
	// to make sure the X is visible and solid
	long long tolerance = width;

	// Check Main Diagonal (top-left to bottom-right)
	bool mainDiag = std::abs(x_h - y_w) < tolerance;

	// Check Anti-Diagonal (bottom-left to top-right)
	bool antiDiag = std::abs(x_h - inv_y_w) < tolerance;

	return mainDiag || antiDiag;
}
struct CompareBlobs {
    bool operator()(const Blob& a, const Blob& b) const {
        // Use std::abs to ensure compatibility with libc++
        if (std::abs((float)a.y - (float)b.y) > 50.0f) {
            return a.y < b.y;
        }
        return a.x < b.x;
    }};
void ImageScaler::processing(float* buf, int width, int height)
{
	if (!_activeStatus)
	{
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				if (isRedXPixel(x, y, width, height))
				{
					Color3 red;
					red.r = 1;
					red.g = 0;
					red.b = 0;
					PlotPixel(buf, width, height, x, y, red);
				}
			}
		}
		return;
	}
	int marginX = width / 20;  // Smaller margin for more coverage
	int marginY = height / 20;

	int step = (_maxObjects > 30) ? 2 : 4;  // Finer scan for high object counts
	float clusterRadius = (_maxObjects > 30) ? 20.0f : 40.0f;  // Smaller clusters for chaos mode

	// Target RGB values
	float tgtR = _tgt.r;
	float tgtG = _tgt.g;
	float tgtB = _tgt.b;

	// STEP 1: Find ALL matching pixels
	std::vector<std::pair<float, float> > matchingPixels;

	for (int py = marginY; py < height - marginY; py += step)
	{
		for (int px = marginX; px < width - marginX; px += step)
		{
			int index = py * width + px;
			int dr = 255 * (buf[index * 4 + 0] - tgtR);
			int dg = 255 * (buf[index * 4 + 1] - tgtG);
			int db = 255 * (buf[index * 4 + 2] - tgtB);
			float dist = sqrtf((float)(dr * dr + dg * dg + db * db));
			if (dist < _tolerance) {
				matchingPixels.push_back(std::make_pair( (float)px, (float)py ));
			}
		}
	}

	// STEP 2: Cluster matching pixels into blobs (find centroids)
	std::vector<Blob> detectedBlobs;
	std::vector<bool> used(matchingPixels.size(), false);

	for (size_t i = 0; i < matchingPixels.size(); i++) {
		if (used[i]) continue;

		// Start a new cluster
		float sumX = matchingPixels[i].first;
		float sumY = matchingPixels[i].second;
		int count = 1;
		used[i] = true;

		// Find all nearby pixels and add to cluster
		for (size_t j = i + 1; j < matchingPixels.size(); j++) {
			if (used[j]) continue;
			float dx = matchingPixels[j].first - matchingPixels[i].first;
			float dy = matchingPixels[j].second - matchingPixels[i].second;
			if (sqrtf(dx * dx + dy * dy) < clusterRadius) {
				sumX += matchingPixels[j].first;
				sumY += matchingPixels[j].second;
				count++;
				used[j] = true;
			}
		}

		// Create blob at centroid
		Blob blob;
		blob.x = sumX / count;
		blob.y = sumY / count;
		blob.w = _boxSize;
		blob.h = _boxSize;
		blob.id = ((int)(blob.x / 20) * 31 + (int)(blob.y / 20) * 17) & 0xFFFF;
		detectedBlobs.push_back(blob);
	}

	// STEP 3: Select blobs spread across the frame
	std::vector<Blob> targets;

	if (!detectedBlobs.empty()) {
		// Minimum spacing - scale down for high object counts (chaos mode!)
		float minSpacing;
		if (_maxObjects >= 50) {
			minSpacing = 15.0f;  // Chaos mode - very tight spacing allowed
		}
		else if (_maxObjects >= 20) {
			minSpacing = 30.0f;  // Dense mode
		}
		else {
			minSpacing = fmaxf(width, height) / (float)(_maxObjects + 1) * 0.5f;
			if (minSpacing < 50.0f) minSpacing = 50.0f;
		}

		// First, pick one blob near each region of the frame
		// Divide frame into grid and pick best blob from each cell
		int gridCols = (int)ceilf(sqrtf((float)_maxObjects * width / height));
		int gridRows = (int)ceilf((float)_maxObjects / gridCols);
		float cellW = (float)width / gridCols;
		float cellH = (float)height / gridRows;

		for (int gy = 0; gy < gridRows && (int)targets.size() < _maxObjects; gy++) {
			for (int gx = 0; gx < gridCols && (int)targets.size() < _maxObjects; gx++) {
				float cellCenterX = (gx + 0.5f) * cellW;
				float cellCenterY = (gy + 0.5f) * cellH;

				// Find blob closest to this cell center
				float bestDist = 999999.0f;
				int bestIdx = -1;
				for (size_t i = 0; i < detectedBlobs.size(); i++) {
					float dx = detectedBlobs[i].x - cellCenterX;
					float dy = detectedBlobs[i].y - cellCenterY;
					float d = sqrtf(dx * dx + dy * dy);

					// Check not too close to already selected targets
					bool tooClose = false;
					for (auto& t : targets) {
						float td = sqrtf((detectedBlobs[i].x - t.x) * (detectedBlobs[i].x - t.x) +
							(detectedBlobs[i].y - t.y) * (detectedBlobs[i].y - t.y));
						if (td < minSpacing) { tooClose = true; break; }
					}

					if (!tooClose && d < bestDist) {
						bestDist = d;
						bestIdx = (int)i;
					}
				}

				if (bestIdx >= 0) {
					targets.push_back(detectedBlobs[bestIdx]);
				}
			}
		}

		// If we still need more targets and have detected blobs, add them more aggressively
		if ((int)targets.size() < _maxObjects && detectedBlobs.size() > targets.size()) {
			std::vector<bool> used(detectedBlobs.size(), false);
			for (auto& t : targets) {
				for (size_t i = 0; i < detectedBlobs.size(); i++) {
					if (detectedBlobs[i].x == t.x && detectedBlobs[i].y == t.y) {
						used[i] = true;
						break;
					}
				}
			}
			// Add remaining blobs with minimal spacing check
			for (size_t i = 0; i < detectedBlobs.size() && (int)targets.size() < _maxObjects; i++) {
				if (used[i]) continue;
				bool tooClose = false;
				for (auto& t : targets) {
					float td = sqrtf((detectedBlobs[i].x - t.x) * (detectedBlobs[i].x - t.x) +
						(detectedBlobs[i].y - t.y) * (detectedBlobs[i].y - t.y));
					if (td < minSpacing * 0.5f) { tooClose = true; break; }
				}
				if (!tooClose) {
					targets.push_back(detectedBlobs[i]);
				}
			}
		}
	}

	// STEP 4: Stable matching - each box tracks nearest target consistently
	std::vector<Blob> blobs;
	float maxMovePerFrame = (_speedPercent >= 99.0f) ? 99999.0f : (_speedPercent * 2.0f + 5.0f);
	if (g_cachedBlobs.empty() || targets.empty()) {
		blobs = targets;
		// Assign IDs
		for (size_t i = 0; i < blobs.size(); i++) {
			blobs[i].id = ((int)(blobs[i].x / 20) * 31 + (int)(blobs[i].y / 20) * 17) & 0xFFFF;
		}
	}
	else {
		// Keep existing boxes, update toward new targets
		blobs = g_cachedBlobs;

		// Adjust count
		while (blobs.size() < targets.size()) {
			Blob newBlob = targets[blobs.size()];
			newBlob.id = ((int)(newBlob.x / 20) * 31 + (int)(newBlob.y / 20) * 17) & 0xFFFF;
			blobs.push_back(newBlob);
		}
		while (blobs.size() > targets.size() && !blobs.empty()) {
			blobs.pop_back();
		}

		// Better matching: sort targets by x then y for consistency
		std::vector<Blob> sortedTargets = targets;
		std::sort(sortedTargets.begin(), sortedTargets.end(), CompareBlobs());

		// Sort blobs the same way
		std::sort(blobs.begin(), blobs.end(),CompareBlobs());

		// Now match sorted lists
		for (size_t i = 0; i < blobs.size() && i < sortedTargets.size(); i++) {
			float targetX = sortedTargets[i].x;
			float targetY = sortedTargets[i].y;
			float dx = targetX - blobs[i].x;
			float dy = targetY - blobs[i].y;
			float distance = sqrtf(dx * dx + dy * dy);

			if (distance <= maxMovePerFrame || _speedPercent >= 99.0f) {
				blobs[i].x = targetX;
				blobs[i].y = targetY;
			}
			else {
				float ratio = maxMovePerFrame / distance;
				blobs[i].x += dx * ratio;
				blobs[i].y += dy * ratio;
			}
		}
	}

	// Save for next frame
	g_cachedBlobs = blobs;

	// Track connection count per node
	std::vector<int> connectionCount(blobs.size(), 0);

	Color3 triColors[3];
	triColors[0] = _nodeClr;
	triColors[1] = _triClr2;
	triColors[2] = _triClr3;

	// Only draw if enabled and not "None"
	if (_linesEnabled && _connectType != 3) {
		float curvature01 = _lineCurvature / 100.0f;
		bool useCurved = curvature01 > 0.0f;
		float fade01 = _fadeByDistance / 100.0f;
		if (fade01 < 0.0f) fade01 = 0.0f;
		if (fade01 > 1.0f) fade01 = 1.0f;
		float invMaxDist = (_maxDist > 0.0f) ? (1.0f / _maxDist) : 0.0f;
		for (size_t i = 0; i < blobs.size(); i++) {
			for (size_t j = i + 1; j < blobs.size(); j++) {
				// Check if either node has reached max connections
				if (connectionCount[i] >= _maxConnections || connectionCount[j] >= _maxConnections) {
					continue;
				}

				// Calculate distance between nodes
				float dx = blobs[j].x - blobs[i].x;
				float dy = blobs[j].y - blobs[i].y;
				float dist = sqrtf(dx * dx + dy * dy);

				// Skip if beyond max distance
				if (dist > _maxDist) {
					continue;
				}

				// Fade based on normalized distance (0..1)
				float fadeFactor = 1.0f - fade01 * (dist * invMaxDist);
				if (fadeFactor < 0.0f) fadeFactor = 0.0f;
				if (fadeFactor > 1.0f) fadeFactor = 1.0f;
				bool drawThisConnection = fadeFactor > 0.001f;

				// Apply connection chance (use deterministic pseudo-random based on IDs for stability)
				if (_connectChance < 1.0f) {
					int seed = (blobs[i].id * 31 + blobs[j].id * 17) & 0xFFFF;
					float chance = (seed % 1000) / 1000.0f;
					if (chance > _connectChance) {
						continue;
					}
				}

				// Draw the connection - pick color based on first node for tri-tone
				Color3 thisLineColor = _lineClr;
				if (_triToneEnabled) {
					int colorIdx = (blobs[i].id * 13 + (int)i * 11) % 3;
					thisLineColor = triColors[colorIdx];
				}

				// Apply fade by distance to the line color (no alpha blending in this buffer).
				thisLineColor.r *= fadeFactor;
				thisLineColor.g *= fadeFactor;
				thisLineColor.b *= fadeFactor;

				if (drawThisConnection) {
					if (_connectType == 1) {
						// Solid lines
						int curveSign = ((blobs[i].id + blobs[j].id) & 1) ? -1 : 1;
						if (useCurved) {
							DrawCurvedLine(buf, width, height, (int)blobs[i].x, (int)blobs[i].y,
								(int)blobs[j].x, (int)blobs[j].y, thisLineColor, curvature01, curveSign);
						}
						else {
							DrawLine(buf, width, height, (int)blobs[i].x, (int)blobs[i].y,
								(int)blobs[j].x, (int)blobs[j].y, thisLineColor);
						}
					}
					else if (_connectType == 2) {
						// Dotted lines (8px dash, 8px gap)
						int curveSign = ((blobs[i].id + blobs[j].id) & 1) ? -1 : 1;
						if (useCurved) {
							DrawCurvedDottedLine(buf, width, height, (int)blobs[i].x, (int)blobs[i].y,
								(int)blobs[j].x, (int)blobs[j].y, thisLineColor, 8, 8, curvature01, curveSign);
						}
						else {
							DrawDottedLine(buf, width, height, (int)blobs[i].x, (int)blobs[i].y,
								(int)blobs[j].x, (int)blobs[j].y, thisLineColor, 8, 8);
						}
					}
				}

				// Increment connection counts
				connectionCount[i]++;
				connectionCount[j]++;
			}
		}
	}

	// STEP 4: Draw shapes and labels
	int fillAlpha = (int)(_fillOpacity * 2.55f);  // 0-255
	if (_textScale < 1) _textScale = 1;
	float currentTime = _time;

	for (size_t i = 0; i < blobs.size(); i++) {
		Blob& b = blobs[i];

		// Pick color for this node (tri-tone or single)
		Color3 thisNodeColor = _nodeClr;
		if (_triToneEnabled) {
			int colorIdx = (b.id * 17 + (int)i * 7) % 3;
			thisNodeColor = triColors[colorIdx];
		}

		// Create dramatic size variation - some boxes much larger than others
		int sizeClass = (b.id * 7) % 10;  // 0-9
		float sizeVariation;
		if (sizeClass < 2) {
			sizeVariation = 2.5f + ((b.id % 50) / 50.0f) * 1.5f;
		}
		else if (sizeClass < 5) {
			sizeVariation = 1.2f + ((b.id % 40) / 40.0f) * 0.8f;
		}
		else {
			sizeVariation = 0.5f + ((b.id % 30) / 30.0f) * 0.5f;
		}

		float baseSize = _boxSize * sizeVariation;

		// Oscillate between taller and wider over time
		float phase = (float)i * 0.7f;
		float oscillation = sinf(currentTime * 1.5f + phase);

		float aspectRange = (sizeClass < 2) ? 0.5f : 0.35f;
		float aspectShift = 1.0f + oscillation * aspectRange;

		int halfW = (int)(baseSize * aspectShift / 2.0f);
		int halfH = (int)(baseSize / aspectShift / 2.0f);

		// Minimum sizes
		if (halfW < 15) halfW = 15;
		if (halfH < 12) halfH = 12;

		int radius = (halfW + halfH) / 2;

		// Draw shape based on selection
		switch (_nodeShape) {
		case 1:  // Filled Circle
			DrawFilledCircle(buf,width,height, (int)b.x, (int)b.y, radius, thisNodeColor, fillAlpha);
			break;
		case 2:  // Circle Outline
			DrawCircleOutline(buf, width, height, (int)b.x, (int)b.y, radius, thisNodeColor);
			break;
		case 3:  // Filled Box
			DrawFilledBox(buf, width, height, (int)b.x, (int)b.y, halfW, halfH, thisNodeColor, fillAlpha);
			break;
		case 4:  // Outlined Box
		{
			// Check if this box should have inverted fill
			bool shouldInvert = false;
			if (_invertFillPercent >= 100.0f) {
				shouldInvert = true;  // 100% = all boxes inverted
			}
			else if (_invertFillPercent > 0.0f) {
				// Use deterministic pseudo-random based on node ID for stability
				int invertRand = (b.id * 59 + (int)i * 43) % 100;
				shouldInvert = (invertRand < (int)_invertFillPercent);
			}

			// Invert the inside of the box first (before drawing outline)
			if (shouldInvert) {
				InvertBoxRegion(buf,width,height, (int)b.x, (int)b.y, halfW, halfH);
			}

			// Then draw the box outline on top
			DrawBox(buf, width, height, (int)b.x, (int)b.y, halfW, halfH, thisNodeColor);
		}
		break;
		case 5:  // Text Only - no shape drawn
			break;
		case 6:  // None - no shape or label
			continue;  // Skip label too
		}

		// Draw label (if enabled and not None shape)
		// Label Density controls what percentage of nodes show labels
		bool showThisLabel = false;

		if (_showLabels && _labelDensity > 0) {
			if (_labelDensity >= 100.0f) {
				showThisLabel = true;  // 100% = all labels
			}
			else {
				// Use deterministic random based on node ID for stability
				int labelRand = (b.id * 73 + (int)i * 37) % 100;
				showThisLabel = (labelRand < (int)_labelDensity);
			}
		}

		if (showThisLabel) {
			int labelX, labelY;
			int actualTextScale = _textScale;

			if (_nodeShape == 5) {
				// Text Only - vary size per node for visual interest
				int sizeVar = (b.id * 13 + (int)i * 7) % 100;
				float scaleMult;
				if (sizeVar < 15) {
					scaleMult = 2.5f + (sizeVar / 15.0f) * 1.0f;  // Large: 2.5x - 3.5x
				}
				else if (sizeVar < 40) {
					scaleMult = 1.5f + ((sizeVar - 15) / 25.0f) * 0.8f;  // Medium: 1.5x - 2.3x
				}
				else if (sizeVar < 70) {
					scaleMult = 1.0f + ((sizeVar - 40) / 30.0f) * 0.4f;  // Normal: 1.0x - 1.4x
				}
				else {
					scaleMult = 0.6f + ((sizeVar - 70) / 30.0f) * 0.3f;  // Small: 0.6x - 0.9x
				}
				actualTextScale = (int)(_textScale * scaleMult);
				if (actualTextScale < 1) actualTextScale = 1;

				// Center text at the tracking point
				int textWidth = 6 * 4 * actualTextScale;
				labelX = (int)b.x - textWidth / 2;
				labelY = (int)b.y - (5 * actualTextScale) / 2;
			}
			else {
				// Other shapes - text above the shape
				labelX = (int)b.x - halfW;
				labelY = (int)b.y - halfH - 8 * actualTextScale;
			}
			DrawLabel(buf, width, height, b.id, (int)i, currentTime, labelX, labelY, _textClr, actualTextScale, _labelType);
		}
	}
}
void ImageScaler::multiThreadProcessImages(OfxRectI p_ProcWindow)
{
    for (int y = p_ProcWindow.y1; y < p_ProcWindow.y2; ++y)
    {
        if (_effect.abort()) break;

        float* dstPix = static_cast<float*>(_dstImg->getPixelAddress(p_ProcWindow.x1, y));

        for (int x = p_ProcWindow.x1; x < p_ProcWindow.x2; ++x)
        {
            float* srcPix = static_cast<float*>(_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

            // do we have a source image to scale up
            if (srcPix)
            {
                for(int c = 0; c < 4; ++c)
                {
                    dstPix[c] = srcPix[c];
                }
            }
            else
            {
                // no src pixel here, be black and transparent
                for (int c = 0; c < 4; ++c)
                {
                    dstPix[c] = 0;
                }
            }

            // increment the dst pixel
            dstPix += 4;
        }
    }
}

void ImageScaler::setSrcImg(OFX::Image* p_SrcImg)
{
    _srcImg = p_SrcImg;
}

void ImageScaler::setParams(double gloalPoints, double redPoints, double greenPoints, double bluePoints, int style, int colorspace, bool bexposure)
{
	_globalPoints = gloalPoints;
	_redPoints = redPoints;
	_greenPoints = greenPoints;
	_bluePoints = bluePoints;
	_style = style;
	_colorspace = colorspace;
	_exposure = bexposure;
}

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class TrackerLabsResolvePlugin : public OFX::ImageEffect
{
public:
    explicit TrackerLabsResolvePlugin(OfxImageEffectHandle p_Handle);
	~TrackerLabsResolvePlugin();
    /* Override the render */
    virtual void render(const OFX::RenderArguments& p_Args);

    /* Override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime);

    /* Override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName);

    /* Override changed clip */
    virtual void changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName);

	/* Set up and run a processor */
    void setupAndProcess(ImageScaler &p_ImageScaler, const OFX::RenderArguments& p_Args);

	void loadOpenCLProgram();
private:
    // Does not own the following pointers
    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;

	
    
	
	char* m_openclProgram;

	OFX::StringParam* m_licenseCode;
	OFX::StringParam* m_licenseStatus;

	OFX::DoubleParam* m_masterIntensity;
	OFX::RGBParam* m_targetClor;
	OFX::DoubleParam* m_colorTolerance;
	OFX::DoubleParam* m_minBlozeSize;
	OFX::IntParam* m_maxObjects;
	OFX::DoubleParam* m_trackingSpeed;
	OFX::BooleanParam* m_ShowLines;
	OFX::ChoiceParam* m_connectionsType;
	OFX::DoubleParam* m_MaxDistance;
	OFX::IntParam* m_maxConnections;
	OFX::DoubleParam* m_connectionChance;
	OFX::BooleanParam* m_triTone;
	OFX::RGBParam* m_trintoneColor2;
	OFX::RGBParam* m_trintoneColor3;
	OFX::DoubleParam* m_lineWidth;
	OFX::RGBParam* m_lineColor;
	OFX::DoubleParam* m_lineCurvature;
	OFX::DoubleParam* m_fadeByDistance;
	OFX::DoubleParam* m_nodeSize;
	OFX::RGBParam* m_nodeColor;
	OFX::ChoiceParam* m_nodeSahpe;
	OFX::DoubleParam* m_fillOpacity;
	OFX::DoubleParam* m_boxPadding;
	OFX::BooleanParam* m_uniformBoxSize;
	OFX::DoubleParam* m_targetSize;
	OFX::DoubleParam* m_labelDensity;
	OFX::ChoiceParam* m_labelContent;
	OFX::BooleanParam* m_showLabels;
	OFX::DoubleParam* m_textSize;
	OFX::RGBParam* m_textColor;
	OFX::DoubleParam* m_invertFill;
	bool m_activeStatus;
};

TrackerLabsResolvePlugin::TrackerLabsResolvePlugin(OfxImageEffectHandle p_Handle)
    : ImageEffect(p_Handle)
{
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);

	m_connectionsType = fetchChoiceParam(ConnectType);
	m_MaxDistance = fetchDoubleParam(MaxDistance);
	m_ShowLines = fetchBooleanParam(ShowLines);

	m_masterIntensity=fetchDoubleParam(kParamMasterIntensity);
	m_targetClor = fetchRGBParam(TargetColor);
	m_colorTolerance=fetchDoubleParam(ColorTolerance);
	m_minBlozeSize=fetchDoubleParam(MinBlobSize);
	m_maxObjects=fetchIntParam(MaxObjects);
	m_trackingSpeed=fetchDoubleParam(TrackingSpeed);

	m_maxConnections=fetchIntParam(MaxConnections);
	m_connectionChance=fetchDoubleParam(ConnectionChance);
	m_triTone=fetchBooleanParam(TriToneColors);
	m_trintoneColor2=fetchRGBParam(TriToneColor2);
	m_trintoneColor3=fetchRGBParam(TriToneColor3);
	m_lineWidth=fetchDoubleParam(LineWidth);
	m_lineColor=fetchRGBParam(LineColor);
	m_lineCurvature=fetchDoubleParam(LineCurvature);
	m_fadeByDistance=fetchDoubleParam(FadeByDistance);
	m_nodeSize=fetchDoubleParam(NodeSize);
	m_nodeColor=fetchRGBParam(NodeColor);
	m_nodeSahpe=fetchChoiceParam(NodeShape);
	m_fillOpacity=fetchDoubleParam(FillOpactity);
	m_boxPadding=fetchDoubleParam(BoxPadding);
	m_uniformBoxSize=fetchBooleanParam(UniformBoxSize);
	m_targetSize=fetchDoubleParam(TargetSize);
	m_labelDensity =fetchDoubleParam(LabelDensity);
	m_labelContent=fetchChoiceParam(LabelContent);
	m_showLabels=fetchBooleanParam(ShowLabels);
	m_textSize=fetchDoubleParam(TextSize);
	m_textColor=fetchRGBParam(TextColor);
	m_invertFill=fetchDoubleParam(InvertFill);

	m_openclProgram = NULL;
	loadOpenCLProgram();

	m_licenseCode = fetchStringParam(kParamNewLicenseCode);
	m_licenseStatus = fetchStringParam(kParamLicenseStatus);

	m_activeStatus = useVerifyOffline()==0 ? true:false;

	string str = "";

	if (m_activeStatus) str = "Licesne was activated.";
	else str = "Please activate license.";

	m_licenseStatus->setValue(str);
}
TrackerLabsResolvePlugin::~TrackerLabsResolvePlugin()
{
	if (m_openclProgram)
	{
		free(m_openclProgram);
		m_openclProgram = NULL;
	}
}
#define fopen_s(pFile, filename, mode) ((*(pFile) = fopen(filename, mode)) == NULL)
void TrackerLabsResolvePlugin::loadOpenCLProgram()
{

#ifdef MAKEPACKCL
	PackOpenCL(kPluginID, CL_PATH, PACKCL_PATH);
#endif
	
	FILE* clFile;
#ifdef PACKCL
	if (fopen_s(&clFile, PACKCL_PATH, "rb") == 0)
#else
	if (fopen_s(&clFile, CL_PATH, "rb") == 0)
#endif
	{
		fseek(clFile, 0, SEEK_END);
		int fsize = ftell(clFile);
		fseek(clFile, 0, SEEK_SET);

		m_openclProgram = (char*)malloc(fsize + 1);
		size_t readCount = fread(m_openclProgram, fsize, 1, clFile);
		if (readCount != 1)
		{
			free(m_openclProgram);
			m_openclProgram = NULL;
		}
		else
		{
#ifdef PACKCL
			//UnPackCLContent(kPluginID, m_openclProgram, fsize);
#endif
			m_openclProgram[fsize] = '\0';
		}
		fclose(clFile);
	}
}
void TrackerLabsResolvePlugin::render(const OFX::RenderArguments& p_Args)
{
    if ((m_DstClip->getPixelDepth() == OFX::eBitDepthFloat) && (m_DstClip->getPixelComponents() == OFX::ePixelComponentRGBA))
    {
        ImageScaler imageScaler(*this);
        setupAndProcess(imageScaler, p_Args);
    }
    else
    {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

bool TrackerLabsResolvePlugin::isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime)
{
	return true;
}
void trim(std::string& s) {
	// Trim from start
	size_t first = s.find_first_not_of(" \t\n\r");
	if (std::string::npos == first) {
		s.clear();
		return;
	}
	// Trim from end
	size_t last = s.find_last_not_of(" \t\n\r");
	s = s.substr(first, (last - first + 1));
}
std::string strip_zero_width(const std::string& input) {
	// UTF-8 hex patterns for common zero-width characters:
	// U+200B (Zero Width Space): \xE2\x80\x8B
	// U+200C (Zero Width Non-Joiner): \xE2\x80\x8C
	// U+200D (Zero Width Joiner): \xE2\x80\x8D
	// U+FEFF (Byte Order Mark): \xEF\xBB\xBF

	// Create a regex to match any of these byte sequences
	std::regex zw_regex("(\xE2\x80[\x8B-\x8D]|\xEF\xBB\xBF)");

	// Replace all matches with an empty string
	return std::regex_replace(input, zw_regex, "");
}
void TrackerLabsResolvePlugin::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName)
{
	if (p_ParamName == kParamLicenseRegister)
	{
		string license;

		m_licenseCode->getValueAtTime(p_Args.time, license);

		trim(license);
		strip_zero_width(license);
		license = std::regex_replace(license, std::regex("\\s+"), "");

		std::string prefix = "TRACK";
		std::string suffix = "TT";
		std::string patternStr = prefix + "([A-Z0-9]{16})" + suffix;
		std::regex licensePattern(patternStr);

		int res_code = 501;
		if (std::regex_match(license, licensePattern))
		{
			res_code = useVerifyOnline(license.c_str(), true);
		}
		if (res_code == 0) m_activeStatus = true;

		string status = "";

		switch (res_code)
		{
		case 501:
			status = "Invalid license key format";
			break;
		case 500:
			status = "Unknown error verifying license. Please contact support.";
			break;
		case 502:
			status = "License key not found.";
			break;
		case 509:
			status = "Maximum number of activations reached.";
			break;
		case 503:
			status = "Network error. Check your internet connection.";
			break;
		case 0:
			status = "Licesne was activated.";
			break;
		}

		m_licenseStatus->setValue(status);
	}
}

void TrackerLabsResolvePlugin::changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName)
{
    if (p_ClipName == kOfxImageEffectSimpleSourceClipName)
    {

    }
}

void TrackerLabsResolvePlugin::setupAndProcess(ImageScaler& p_ImageScaler, const OFX::RenderArguments& p_Args)
{
    // Get the dst image
    std::auto_ptr<OFX::Image> dst(m_DstClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth();
    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();

    // Get the src image
    std::auto_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum srcBitDepth = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // Check to see if the bit depth and number of components are the same
    if ((srcBitDepth != dstBitDepth) || (srcComponents != dstComponents))
    {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }

	bool bThumb = m_SrcClip->isForThumbnail();

	double time = p_Args.time;
	int connections_type = 0;
	m_connectionsType->getValueAtTime(time, connections_type);
	double maxDistance = m_MaxDistance->getValueAtTime(time);

    // Set the images
	p_ImageScaler._time = time;
	double r, g, b;
	m_targetClor->getValueAtTime(time, r, g, b);
	p_ImageScaler._tgt.r = r;
	p_ImageScaler._tgt.g = g;
	p_ImageScaler._tgt.b = b;

	double val;
	m_colorTolerance->getValueAtTime(time, val);
	p_ImageScaler._tolerance = val;

	p_ImageScaler._maxObjects= m_maxObjects->getValueAtTime(time);
	// NodeSize acts as a multiplicative scale factor for node size.
	p_ImageScaler._boxSize = m_targetSize->getValueAtTime(time);
	p_ImageScaler._triToneEnabled = m_triTone->getValueAtTime(time);
	p_ImageScaler._nodeOutlineWidth = m_nodeSize->getValueAtTime(time);
	m_lineColor->getValueAtTime(time, r, g, b);
	p_ImageScaler._lineClr.r = r;
	p_ImageScaler._lineClr.g = g;
	p_ImageScaler._lineClr.b = b;
	p_ImageScaler._lineWidth = m_lineWidth->getValueAtTime(time);
	p_ImageScaler._lineCurvature = m_lineCurvature->getValueAtTime(time);
	p_ImageScaler._fadeByDistance = m_fadeByDistance->getValueAtTime(time);

	m_nodeColor->getValueAtTime(time, r, g, b);
	p_ImageScaler._nodeClr.r = r;
	p_ImageScaler._nodeClr.g = g;
	p_ImageScaler._nodeClr.b = b;

	m_trintoneColor2->getValueAtTime(time, r, g, b);
	p_ImageScaler._triClr2.r = r;
	p_ImageScaler._triClr2.g = g;
	p_ImageScaler._triClr2.b = b;

	m_trintoneColor3->getValueAtTime(time, r, g, b);
	p_ImageScaler._triClr3.r = r;
	p_ImageScaler._triClr3.g = g;
	p_ImageScaler._triClr3.b = b;

	p_ImageScaler._speedPercent = m_trackingSpeed->getValueAtTime(time);
	p_ImageScaler._linesEnabled = m_ShowLines->getValueAtTime(time);

	int ival;
	m_connectionsType->getValueAtTime(time, ival);
	p_ImageScaler._connectType = ival+1;
	p_ImageScaler._maxDist = m_MaxDistance->getValueAtTime(time);
	p_ImageScaler._maxConnections = m_maxConnections->getValueAtTime(time);
	p_ImageScaler._connectChance = m_connectionChance->getValueAtTime(time);
	m_nodeSahpe->getValueAtTime(time, ival);
	p_ImageScaler._nodeShape = ival + 1;
	p_ImageScaler._fillOpacity = m_fillOpacity->getValueAtTime(time);
	p_ImageScaler._textScale = m_textSize->getValueAtTime(time);
	m_textColor->getValueAtTime(time, r, g, b);
	p_ImageScaler._textClr.r = r;
	p_ImageScaler._textClr.g = g;
	p_ImageScaler._textClr.b = b;
	p_ImageScaler._showLabels = m_showLabels->getValueAtTime(time);
	m_labelContent->getValueAtTime(time, ival);
	p_ImageScaler._labelType = ival;
	p_ImageScaler._invertFillPercent = m_invertFill->getValueAtTime(time);
	p_ImageScaler._labelDensity = m_labelDensity->getValueAtTime(time);

    p_ImageScaler.setDstImg(dst.get());
    p_ImageScaler.setSrcImg(src.get());
	p_ImageScaler.setThumbIcon(bThumb);
    // Setup OpenCL and CUDA Render arguments
    p_ImageScaler.setGPURenderArgs(p_Args);

    // Set the render window
    p_ImageScaler.setRenderWindow(p_Args.renderWindow);

	p_ImageScaler.setOpenCLProgram(m_openclProgram);
	p_ImageScaler.setActiveStatus(m_activeStatus);
    // Call the base class process member, this will call the derived templated process code
    p_ImageScaler.process();
}

////////////////////////////////////////////////////////////////////////////////

using namespace OFX;

TrackerLabsResolvePluginFactory::TrackerLabsResolvePluginFactory()
    : OFX::PluginFactoryHelper<TrackerLabsResolvePluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
{
}

void TrackerLabsResolvePluginFactory::describe(OFX::ImageEffectDescriptor& p_Desc)
{
    // Basic labels
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
    p_Desc.setPluginGrouping(kPluginGrouping);
    p_Desc.setPluginDescription(kPluginDescription);

    // Add the supported contexts, only filter at the moment
    p_Desc.addSupportedContext(eContextFilter);
    p_Desc.addSupportedContext(eContextGeneral);

    // Add supported pixel depths
    p_Desc.addSupportedBitDepth(eBitDepthFloat);

    // Set a few flags
    p_Desc.setSingleInstance(false);
    p_Desc.setHostFrameThreading(false);
    p_Desc.setSupportsMultiResolution(kSupportsMultiResolution);
    p_Desc.setSupportsTiles(kSupportsTiles);
    p_Desc.setTemporalClipAccess(false);
    p_Desc.setRenderTwiceAlways(false);
    p_Desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);

    // Setup OpenCL render capability flags
    p_Desc.setSupportsOpenCLRender(true);

    // Setup CUDA render capability flags on non-Apple system
#ifndef __APPLE__
    p_Desc.setSupportsCudaRender(true);
    p_Desc.setSupportsCudaStream(true);
#endif

    // Setup Metal render capability flags only on Apple system
#ifdef __APPLE__
    p_Desc.setSupportsMetalRender(true);
#endif

    // Indicates that the plugin output does not depend on location or neighbours of a given pixel.
    // Therefore, this plugin could be executed during LUT generation.
    p_Desc.setNoSpatialAwareness(true);
}
void TrackerLabsResolvePluginFactory::describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum /*p_Context*/)
{
    // Source clip only in the filter context
    // Create the mandated source clip
    ClipDescriptor* srcClip = p_Desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // Create the mandated output clip
    ClipDescriptor* dstClip = p_Desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);

    // Make some pages and to things in
	PageParamDescriptor* page = p_Desc.definePageParam("Controls");

	{
		DoubleParamDescriptor* MasterIntensity_param = p_Desc.defineDoubleParam(kParamMasterIntensity);
		MasterIntensity_param->setLabel(kParamMasterIntensityLabel);
		MasterIntensity_param->setRange(0, 100);
		MasterIntensity_param->setDisplayRange(0, 100);
		MasterIntensity_param->setIncrement(0.1);
		MasterIntensity_param->setDefault(100);
		MasterIntensity_param->setDoubleType(eDoubleTypeScale);
		page->addChild(*MasterIntensity_param);
	}

	{
		GroupParamDescriptor* group_param = p_Desc.defineGroupParam(TrackingEngineGroup);
		group_param->setLabel(TrackingEngineGroupLabel);
		page->addChild(*group_param);
		{
			RGBParamDescriptor* targetcolor_param = p_Desc.defineRGBParam(TargetColor);
			targetcolor_param->setLabel(TargetColorLabel);
			targetcolor_param->setDefault(1, 100/255.f, 100 / 255.f);
			targetcolor_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* ColorTolerance_param = p_Desc.defineDoubleParam(ColorTolerance);
			ColorTolerance_param->setLabel(ColorToleranceLabel);
			ColorTolerance_param->setRange(0, 255);
			ColorTolerance_param->setDisplayRange(0, 255);
			ColorTolerance_param->setIncrement(0.1);
			ColorTolerance_param->setDefault(80);
			ColorTolerance_param->setDoubleType(eDoubleTypeScale);
			ColorTolerance_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* ColorTolerance_param = p_Desc.defineDoubleParam(MinBlobSize);
			ColorTolerance_param->setLabel(MinBlobSizeLabel);
			ColorTolerance_param->setRange(0, 500);
			ColorTolerance_param->setDisplayRange(0, 500);
			ColorTolerance_param->setIncrement(0.1);
			ColorTolerance_param->setDefault(45);
			ColorTolerance_param->setDoubleType(eDoubleTypeScale);
			ColorTolerance_param->setParent(*group_param);
		}
		{
			IntParamDescriptor* MaxObjects_param = p_Desc.defineIntParam(MaxObjects);
			MaxObjects_param->setLabel(MaxObjectsLabel);
			MaxObjects_param->setRange(0, 100);
			MaxObjects_param->setDisplayRange(0, 100);
			MaxObjects_param->setDefault(5);
			MaxObjects_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* TrackingSpeed_param = p_Desc.defineDoubleParam(TrackingSpeed);
			TrackingSpeed_param->setLabel(TrackingSpeedLabel);
			TrackingSpeed_param->setRange(0, 100);
			TrackingSpeed_param->setDisplayRange(0, 100);
			TrackingSpeed_param->setIncrement(0.1);
			TrackingSpeed_param->setDefault(100);
			TrackingSpeed_param->setDoubleType(eDoubleTypeScale);
			TrackingSpeed_param->setParent(*group_param);
		}
	}

	{
		GroupParamDescriptor* group_param = p_Desc.defineGroupParam(ConnectionsGroup);
		group_param->setLabel(ConnectionsGroupLabel);
		page->addChild(*group_param);
		{
			BooleanParamDescriptor* ShowLines_param = p_Desc.defineBooleanParam(ShowLines);
			ShowLines_param->setLabel(ShowLinesLabel);
			ShowLines_param->setDefault(true);
			ShowLines_param->setParent(*group_param);
		}

		{
			ChoiceParamDescriptor* connectType_param = p_Desc.defineChoiceParam(ConnectType);
			connectType_param->setLabel(ConnectTypeLabel);
			assert(connectType_param->getNOptions() == CT_LINE);
			connectType_param->appendOption("Lines", "Lines");
			assert(connectType_param->getNOptions() == CT_DOT);
			connectType_param->appendOption("Dotted", "Dotted");
			assert(connectType_param->getNOptions() == CT_NONE);
			connectType_param->appendOption("None", "None");
			connectType_param->setDefault(CT_LINE);
			connectType_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* MaxDistance_param = p_Desc.defineDoubleParam(MaxDistance);
			MaxDistance_param->setLabel(MaxDistanceLabel);
			MaxDistance_param->setRange(0, 2000);
			MaxDistance_param->setDisplayRange(0, 2000);
			MaxDistance_param->setIncrement(0.1);
			MaxDistance_param->setDefault(2000);
			MaxDistance_param->setDoubleType(eDoubleTypeScale);
			MaxDistance_param->setParent(*group_param);
		}
		{
			IntParamDescriptor* MaxConnections_param = p_Desc.defineIntParam(MaxConnections);
			MaxConnections_param->setLabel(MaxConnectionsLabel);
			MaxConnections_param->setRange(0, 20);
			MaxConnections_param->setDisplayRange(0, 20);
			MaxConnections_param->setDefault(10);
			MaxConnections_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* ConnectionChance_param = p_Desc.defineDoubleParam(ConnectionChance);
			ConnectionChance_param->setLabel(ConnectionChanceLabel);
			ConnectionChance_param->setRange(0, 100);
			ConnectionChance_param->setDisplayRange(0, 100);
			ConnectionChance_param->setIncrement(0.1);
			ConnectionChance_param->setDefault(100);
			ConnectionChance_param->setDoubleType(eDoubleTypeScale);
			ConnectionChance_param->setParent(*group_param);
		}
	}
	{
		GroupParamDescriptor* group_param = p_Desc.defineGroupParam(VisualsGroup);
		group_param->setLabel(VisualsGroupLabel);

		{
			BooleanParamDescriptor* TriTone_param = p_Desc.defineBooleanParam(TriToneColors);
			TriTone_param->setLabel(TriToneColorsLabel);
			TriTone_param->setDefault(false);
			TriTone_param->setParent(*group_param);
		}

		{
			RGBParamDescriptor* trintonecolor_param = p_Desc.defineRGBParam(TriToneColor2);
			trintonecolor_param->setLabel(TriToneColor2Label);
			trintonecolor_param->setDefault(0, 200 / 255.f, 1);
			trintonecolor_param->setParent(*group_param);
		}

		{
			RGBParamDescriptor* trintonecolor_param = p_Desc.defineRGBParam(TriToneColor3);
			trintonecolor_param->setLabel(TriToneColor3Label);
			trintonecolor_param->setDefault(1, 100 / 255.f, 200/255.f);
			trintonecolor_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* LineWidth_param = p_Desc.defineDoubleParam(LineWidth);
			LineWidth_param->setLabel(LineWidthLabel);
			LineWidth_param->setRange(0, 50);
			LineWidth_param->setDisplayRange(0, 50);
			LineWidth_param->setIncrement(0.1);
			LineWidth_param->setDefault(1);
			LineWidth_param->setDoubleType(eDoubleTypeScale);
			LineWidth_param->setParent(*group_param);
		}
		{
			RGBParamDescriptor* LineColor_param = p_Desc.defineRGBParam(LineColor);
			LineColor_param->setLabel(LinecolorLabel);
			LineColor_param->setDefault(1, 1, 1);
			LineColor_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* LLineCurvature_param = p_Desc.defineDoubleParam(LineCurvature);
			LLineCurvature_param->setLabel(LineCurvatureLabel);
			LLineCurvature_param->setRange(0, 100);
			LLineCurvature_param->setDisplayRange(0, 100);
			LLineCurvature_param->setIncrement(0.1);
			LLineCurvature_param->setDefault(0);
			LLineCurvature_param->setDoubleType(eDoubleTypeScale);
			LLineCurvature_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* FadeByDistance_param = p_Desc.defineDoubleParam(FadeByDistance);
			FadeByDistance_param->setLabel(FadeByDistanceLabel);
			FadeByDistance_param->setRange(0, 100);
			FadeByDistance_param->setDisplayRange(0, 100);
			FadeByDistance_param->setIncrement(0.1);
			FadeByDistance_param->setDefault(0);
			FadeByDistance_param->setDoubleType(eDoubleTypeScale);
			FadeByDistance_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* NodeSize_param = p_Desc.defineDoubleParam(NodeSize);
			NodeSize_param->setLabel(NodeSizeLabel);
			NodeSize_param->setRange(0, 50);
			NodeSize_param->setDisplayRange(0, 50);
			NodeSize_param->setIncrement(0.1);
			NodeSize_param->setDefault(2);
			NodeSize_param->setDoubleType(eDoubleTypeScale);
			NodeSize_param->setParent(*group_param);
		}
		{
			RGBParamDescriptor* NodeColor_param = p_Desc.defineRGBParam(NodeColor);
			NodeColor_param->setLabel(NodeColorLabel);
			NodeColor_param->setDefault(1, 1,1);
			NodeColor_param->setParent(*group_param);
		}
		{
			ChoiceParamDescriptor* NodeShape_param = p_Desc.defineChoiceParam(NodeShape);
			NodeShape_param->setLabel(NodeShapeLabel);
			assert(NodeShape_param->getNOptions() == NS_FILL_CIRCLE);
			NodeShape_param->appendOption("Filled Circle", "Filled Circle");
			assert(NodeShape_param->getNOptions() == NS_CIRCLE_OUTLINE);
			NodeShape_param->appendOption("Circle Outline", "Circle Outline");
			assert(NodeShape_param->getNOptions() == NS_FILLED_BOX);
			NodeShape_param->appendOption("Filled Box", "Filled Box");
			assert(NodeShape_param->getNOptions() == NS_OUTLINED_BOX);
			NodeShape_param->appendOption("Outlined Box", "Outlined Box");
			assert(NodeShape_param->getNOptions() == NS_TEXT_ONLY);
			NodeShape_param->appendOption("Text Only", "Text Only");
			assert(NodeShape_param->getNOptions() == NS_NONE);
			NodeShape_param->appendOption("None", "None");
			NodeShape_param->setDefault(NS_OUTLINED_BOX);
			NodeShape_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* FillOpactity_param = p_Desc.defineDoubleParam(FillOpactity);
			FillOpactity_param->setLabel(FillOpactityLabel);
			FillOpactity_param->setRange(0, 100);
			FillOpactity_param->setDisplayRange(0, 100);
			FillOpactity_param->setIncrement(0.1);
			FillOpactity_param->setDefault(50);
			FillOpactity_param->setDoubleType(eDoubleTypeScale);
			FillOpactity_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* BoxPadding_param = p_Desc.defineDoubleParam(BoxPadding);
			BoxPadding_param->setLabel(BoxPaddingLabel);
			BoxPadding_param->setRange(0, 200);
			BoxPadding_param->setDisplayRange(0, 200);
			BoxPadding_param->setIncrement(0.1);
			BoxPadding_param->setDefault(0);
			BoxPadding_param->setDoubleType(eDoubleTypeScale);
			BoxPadding_param->setParent(*group_param);
		}

		{
			BooleanParamDescriptor* UniformBoxSize_param = p_Desc.defineBooleanParam(UniformBoxSize);
			UniformBoxSize_param->setLabel(UniformBoxSizeLabel);
			UniformBoxSize_param->setDefault(false);
			UniformBoxSize_param->setParent(*group_param);
		}

		{
			DoubleParamDescriptor* TargetSize_param = p_Desc.defineDoubleParam(TargetSize);
			TargetSize_param->setLabel(TargetSizeLabel);
			TargetSize_param->setRange(0, 300);
			TargetSize_param->setDisplayRange(0, 300);
			TargetSize_param->setIncrement(0.1);
			TargetSize_param->setDefault(60);
			TargetSize_param->setDoubleType(eDoubleTypeScale);
			TargetSize_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* LabelDensity_param = p_Desc.defineDoubleParam(LabelDensity);
			LabelDensity_param->setLabel(LabelDensityLabel);
			LabelDensity_param->setRange(0, 100);
			LabelDensity_param->setDisplayRange(0, 100);
			LabelDensity_param->setIncrement(0.1);
			LabelDensity_param->setDefault(23);
			LabelDensity_param->setDoubleType(eDoubleTypeScale);
			LabelDensity_param->setParent(*group_param);
		}
		{
			ChoiceParamDescriptor* LabelContent_param = p_Desc.defineChoiceParam(LabelContent);
			LabelContent_param->setLabel(LabelContentLabel);
			assert(LabelContent_param->getNOptions() == LC_COORDINATES);
			LabelContent_param->appendOption("Coordinates", "Coordinates");
			assert(LabelContent_param->getNOptions() == LC_RANDOM_HEX);
			LabelContent_param->appendOption("Random Hex", "Random Hex");
			assert(LabelContent_param->getNOptions() == LC_CUSTOM);
			LabelContent_param->appendOption("Custom", "Custom");
			assert(LabelContent_param->getNOptions() == LC_SEQUENTIAL001);
			LabelContent_param->appendOption("Sequential 001", "Sequential 001");
			assert(LabelContent_param->getNOptions() == LC_TIMECODE);
			LabelContent_param->appendOption("Timecode", "Timecode");
			assert(LabelContent_param->getNOptions() == LC_BINARY);
			LabelContent_param->appendOption("Binary", "Binary");
			assert(LabelContent_param->getNOptions() == LC_HEX_MEMORY);
			LabelContent_param->appendOption("Hex Memory", "Hex Memory");
			assert(LabelContent_param->getNOptions() == LC_JAPANESE_TRACK);
			LabelContent_param->appendOption("Japanese Track", "Japanese Track");
			assert(LabelContent_param->getNOptions() == LC_JAPANESE_TARGET);
			LabelContent_param->appendOption("Japanese Target", "Japanese Target");
			assert(LabelContent_param->getNOptions() == LC_KATAKANA);
			LabelContent_param->appendOption("Katakana", "Katakana");
			assert(LabelContent_param->getNOptions() == LC_CHINESE_LOCK);
			LabelContent_param->appendOption("Chinese Lock", "Chinese Lock");
			LabelContent_param->setDefault(LC_RANDOM_HEX);
			LabelContent_param->setParent(*group_param);
		}

		{
			BooleanParamDescriptor* ShowLabels_param = p_Desc.defineBooleanParam(ShowLabels);
			ShowLabels_param->setLabel(ShowLabelsLabel);
			ShowLabels_param->setDefault(true);
			ShowLabels_param->setParent(*group_param);
		}

		{
			DoubleParamDescriptor* TextSize_param = p_Desc.defineDoubleParam(TextSize);
			TextSize_param->setLabel(TextSizeLabel);
			TextSize_param->setRange(0, 20);
			TextSize_param->setDisplayRange(0, 20);
			TextSize_param->setIncrement(0.1);
			TextSize_param->setDefault(2);
			TextSize_param->setDoubleType(eDoubleTypeScale);
			TextSize_param->setParent(*group_param);
		}
		{
			RGBParamDescriptor* TextColor_param = p_Desc.defineRGBParam(TextColor);
			TextColor_param->setLabel(TextColorLabel);
			TextColor_param->setDefault(1, 1, 1);
			TextColor_param->setParent(*group_param);
		}
		{
			DoubleParamDescriptor* InvertFill_param = p_Desc.defineDoubleParam(InvertFill);
			InvertFill_param->setLabel(InvertFillLabel);
			InvertFill_param->setRange(0, 100);
			InvertFill_param->setDisplayRange(0, 100);
			InvertFill_param->setIncrement(0.1);
			InvertFill_param->setDefault(0);
			InvertFill_param->setDoubleType(eDoubleTypeScale);
			InvertFill_param->setParent(*group_param);
		}
	}
  
	{
		GroupParamDescriptor* group_param = p_Desc.defineGroupParam(kParamLicenseGroup);
		group_param->setLabel(kParamLicenseGroupLabel);
		page->addChild(*group_param);
		{
			StringParamDescriptor* license_code = p_Desc.defineStringParam(kParamNewLicenseCode);
			license_code->setLabel(kParamNewLicenseCodeLabel);
			license_code->setParent(*group_param);
		}
		{
			StringParamDescriptor* license_status = p_Desc.defineStringParam(kParamLicenseStatus);
			license_status->setLabel(kParamLicenseStatusLabel);
			license_status->setParent(*group_param);
		}
		{
			PushButtonParamDescriptor* reg = p_Desc.definePushButtonParam(kParamLicenseRegister);
			reg->setLabel(kParamLicenseRegisterLabel);
			reg->setParent(*group_param);
		}
	}
}

ImageEffect* TrackerLabsResolvePluginFactory::createInstance(OfxImageEffectHandle p_Handle, ContextEnum /*p_Context*/)
{
    return new TrackerLabsResolvePlugin(p_Handle);
}

void OFX::Plugin::getPluginIDs(PluginFactoryArray& p_FactoryArray)
{
    static TrackerLabsResolvePluginFactory TrackerLabsResolvePlugin;
    p_FactoryArray.push_back(&TrackerLabsResolvePlugin);
}
