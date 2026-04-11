
//

#define kParamMasterIntensity "MasterIntensity"
#define kParamMasterIntensityLabel "Master Intensity %"

#define TrackingEngineGroup "TrackingEngineGroup"
#define TrackingEngineGroupLabel "Tracking Engine"

#define TargetColor "TargetColor"
#define TargetColorLabel "Target Color"
#define ColorTolerance "ColorTolerance"
#define ColorToleranceLabel "Color Tolerance"
#define MinBlobSize "MinBlobSize"
#define MinBlobSizeLabel "Min Blob Size"
#define MaxObjects "MaxObjects"
#define MaxObjectsLabel "Max Objects"
#define TrackingSpeed "TrackingSpeed"
#define TrackingSpeedLabel "Tracking Speed %"

#define ConnectionsGroup "ConnectionsGroup"
#define ConnectionsGroupLabel "Connections"

#define ShowLines "ShowLines"
#define ShowLinesLabel "Show Lines"

#define ConnectType "ConnectType"
#define ConnectTypeLabel "Type"

enum eConnectType
{
	CT_LINE,
	CT_DOT,
	CT_NONE
};

#define MaxDistance "MaxDistance"
#define MaxDistanceLabel "Max Distance"
#define MaxConnections "MaxConnections"
#define MaxConnectionsLabel "Max Connections"
#define ConnectionChance "ConnectionChance"
#define ConnectionChanceLabel "Connection Chance %"

#define VisualsGroup "Visuals"
#define VisualsGroupLabel "Visuals"

#define TriToneColors "TriToneColors"
#define TriToneColorsLabel "Tri-Tone Colors"

#define TriToneColor2 "TriToneColor2"
#define TriToneColor2Label "Color 2"
#define TriToneColor3 "TriToneColor3"
#define TriToneColor3Label "Color 3"

#define LineWidth "LineWidth"
#define LineWidthLabel "Line Width"
#define LineColor "Linecolor"
#define LinecolorLabel "Line Color"

#define LineCurvature "LineCurvature"
#define LineCurvatureLabel "Line Curvature %"
#define FadeByDistance "FadeByDistance"
#define FadeByDistanceLabel "Fade by Distance %"
#define NodeSize "NodeSize"
#define NodeSizeLabel "Node Size"
#define NodeColor "NodeColor"
#define NodeColorLabel "Node Color"
#define NodeShape "NodeShape"
#define NodeShapeLabel "Node Shape"

enum eNodeShape
{
	NS_FILL_CIRCLE,
	NS_CIRCLE_OUTLINE,
	NS_FILLED_BOX,
	NS_OUTLINED_BOX,
	NS_TEXT_ONLY,
	NS_NONE
};

#define FillOpactity "FillOpacity"
#define FillOpactityLabel "Fill Opacity %"
#define BoxPadding "BoxPadding"
#define BoxPaddingLabel "BoxPadding %"
#define UniformBoxSize "UniformBoxSize"
#define UniformBoxSizeLabel "Uniform Box Size"
#define TargetSize "TargetSize"
#define TargetSizeLabel "Target Size"
#define LabelDensity "LabelDensity"
#define LabelDensityLabel "Label Density %"
#define LabelContent "LabelContent"
#define LabelContentLabel "Label Content"

enum eLabelContent
{
	LC_COORDINATES,
	LC_RANDOM_HEX,
	LC_CUSTOM,
	LC_SEQUENTIAL001,
	LC_TIMECODE,
	LC_BINARY,
	LC_HEX_MEMORY,
	LC_JAPANESE_TRACK,
	LC_JAPANESE_TARGET,
	LC_KATAKANA,
	LC_CHINESE_LOCK
};

#define ShowLabels "ShowLabels"
#define ShowLabelsLabel "Show Labels"
#define TextSize "TextSize"
#define TextSizeLabel "Text Size"
#define TextColor "TextColor"
#define TextColorLabel "Text Color"
#define InvertFill "InvertFill"
#define InvertFillLabel "Invert Fill %"
//#define MAKEPACKCL
#define PACKCL

#define kPluginID 2

#define kPluginName "TrackerLabs"
#define kPluginGrouping "TinyTapes"
#define kPluginDescription ""
#define kPluginIdentifier "TrackerLabs"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

#define kParamLicenseGroup "License group"
#define kParamLicenseGroupLabel "License"

#define kParamNewLicenseCode "New license code"
#define kParamNewLicenseCodeLabel "License"

#define kParamLicenseStatus "License Status"
#define kParamLicenseStatusLabel ""

#define kParamLicenseRegister "Register"
#define kParamLicenseRegisterLabel "Register"

struct Color3
{
	float r, g, b;
};
// --- BLOB STRUCTURE ---
struct Blob {
	float x, y;
	float w, h;
	int id;
	float targetX, targetY;  // For smooth interpolation
};

// Simple 3x5 digit font
static const unsigned char DIGITS[10][5] = {
	{0x7,0x5,0x5,0x5,0x7}, {0x2,0x6,0x2,0x2,0x7}, {0x7,0x1,0x7,0x4,0x7},
	{0x7,0x1,0x7,0x1,0x7}, {0x5,0x5,0x7,0x1,0x1}, {0x7,0x4,0x7,0x1,0x7},
	{0x7,0x4,0x7,0x5,0x7}, {0x7,0x1,0x1,0x1,0x1}, {0x7,0x5,0x7,0x5,0x7},
	{0x7,0x5,0x7,0x1,0x7}
};
static const unsigned char HEX_CHARS[6][5] = {
	{0x2,0x5,0x7,0x5,0x5}, {0x6,0x5,0x6,0x5,0x6}, {0x3,0x4,0x4,0x4,0x3},
	{0x6,0x5,0x5,0x5,0x6}, {0x7,0x4,0x6,0x4,0x7}, {0x7,0x4,0x6,0x4,0x4}
};

// 8x8 pixel art for CJK characters (each row is a byte, MSB left)
// Japanese Kanji
static const unsigned char KANJI_TSUI[8] = { 0x00,0x7E,0x42,0x7E,0x42,0x7E,0x24,0x42 };  // ?
static const unsigned char KANJI_SEKI[8] = { 0x20,0x7C,0x44,0xFE,0x44,0x7C,0x44,0x44 };  // ?
static const unsigned char KANJI_HYO[8] = { 0x10,0xFE,0x38,0x54,0xFE,0x10,0x38,0x54 };   // ?
static const unsigned char KANJI_TEKI[8] = { 0x78,0x48,0x78,0x48,0x78,0x0A,0x7A,0x0A };  // ?
static const unsigned char KANJI_SUO[8] = { 0x24,0x7E,0x24,0x00,0x7E,0x42,0x7E,0x42 };   // ?
static const unsigned char KANJI_DING[8] = { 0xFE,0x10,0x7C,0x10,0xFE,0x10,0x10,0x10 };  // ?
// Extra kanji for variety
static const unsigned char KANJI_SEN[8] = { 0x10,0x10,0xFE,0x10,0x10,0xFE,0x10,0x10 };   // ? (line)
static const unsigned char KANJI_TEN[8] = { 0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00 };   // ? (point)
static const unsigned char KANJI_KEN[8] = { 0x10,0x7C,0x54,0x7C,0x54,0x7C,0x10,0x10 };   // ? (see)
static const unsigned char KANJI_SHI[8] = { 0x7E,0x40,0x7E,0x02,0x02,0x7E,0x00,0x00 };   // ? (view)
static const unsigned char KANJI_MEI[8] = { 0x24,0x7E,0x24,0x7E,0x24,0x7E,0x24,0x00 };   // ? (bright)
static const unsigned char KANJI_AN[8] = { 0xFE,0x82,0xBA,0xAA,0xBA,0x82,0xFE,0x00 };    // ? (dark)

// Katakana characters
static const unsigned char KATA_TO[8] = { 0x40,0x40,0x7C,0x40,0x20,0x10,0x08,0x00 };     // ?
static const unsigned char KATA_RA[8] = { 0x7E,0x02,0x02,0x7E,0x02,0x04,0x08,0x30 };     // ?
static const unsigned char KATA_TSU[8] = { 0x00,0x24,0x42,0x00,0x00,0x08,0x10,0x00 };    // ?
static const unsigned char KATA_KU[8] = { 0x3E,0x22,0x02,0x04,0x08,0x10,0x20,0x00 };     // ?
static const unsigned char KATA_RU[8] = { 0x7E,0x42,0x42,0x7E,0x04,0x08,0x10,0x20 };     // ?
static const unsigned char KATA_BU[8] = { 0x7E,0x08,0x7E,0x08,0x08,0x14,0x22,0x00 };     // ?
static const unsigned char KATA_RO[8] = { 0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00 };     // ?
static const unsigned char KATA_N[8] = { 0x00,0x42,0x42,0x22,0x12,0x0A,0x06,0x02 };      // ?
static const unsigned char KATA_A[8] = { 0x7E,0x08,0x08,0x7E,0x08,0x14,0x22,0x00 };      // ?
static const unsigned char KATA_I[8] = { 0x00,0x24,0x24,0x24,0x04,0x08,0x10,0x00 };      // ?
static const unsigned char KATA_SU[8] = { 0x7E,0x08,0x08,0x08,0x14,0x22,0x41,0x00 };     // ?
static const unsigned char KATA_KA[8] = { 0x10,0x7E,0x12,0x12,0x22,0x42,0x02,0x00 };     // ?
static const unsigned char KATA_TE[8] = { 0x7E,0x00,0x7E,0x10,0x10,0x10,0x10,0x10 };     // ?
static const unsigned char KATA_PU[8] = { 0x24,0x00,0x7E,0x08,0x08,0x14,0x22,0x00 };     // ?

// Word arrays for variety (pointers to character pairs/quads)
struct KanjiWord { const unsigned char* chars[4]; int len; };
static const KanjiWord JP_TRACK_WORDS[] = {
	{{KANJI_TSUI, KANJI_SEKI, NULL, NULL}, 2},  // ?? track
	{{KANJI_SEN, KANJI_TEN, NULL, NULL}, 2},    // ?? line-point
	{{KANJI_KEN, KANJI_SHI, NULL, NULL}, 2},    // ?? see-view
	{{KANJI_MEI, KANJI_AN, NULL, NULL}, 2},     // ?? light-dark
};
static const KanjiWord JP_TARGET_WORDS[] = {
	{{KANJI_HYO, KANJI_TEKI, NULL, NULL}, 2},   // ?? target
	{{KANJI_TEN, KANJI_KEN, NULL, NULL}, 2},    // ?? point-see
	{{KANJI_SHI, KANJI_SEN, NULL, NULL}, 2},    // ?? line of sight
};
static const KanjiWord KATA_WORDS[] = {
	{{KATA_TO, KATA_RA, KATA_TSU, KATA_KU}, 4}, // ???? track
	{{KATA_RU, KATA_BU, KATA_SU, NULL}, 3},     // ??? labs
	{{KATA_BU, KATA_RO, KATA_N, NULL}, 3},      // ??? blob
	{{KATA_RA, KATA_I, KATA_N, NULL}, 3},       // ??? line
	{{KATA_TE, KATA_PU, KATA_SU, NULL}, 3},     // ??? tapes
	{{KATA_SU, KATA_KA, KATA_N, NULL}, 3},      // ??? scan
};
static const KanjiWord CN_LOCK_WORDS[] = {
	{{KANJI_SUO, KANJI_DING, NULL, NULL}, 2},   // ?? lock
	{{KANJI_KEN, KANJI_TSUI, NULL, NULL}, 2},   // ?? see-chase
	{{KANJI_SEN, KANJI_MEI, NULL, NULL}, 2},    // ?? line-bright
};
// English word variations for sequential labels
static const char* SEQ_WORDS[] = { "TRK", "LAB", "BLB", "LNE", "TPE", "NOD", "PTR", "SRC" };
static const int SEQ_WORDS_COUNT = 8;
#define CL_PATH "C:\\Program Files\\Common Files\\OFX\\Plugins\\PointsPlugin.ofx.bundle\\Contents\\Resources\\PointsPluginCLKernel.cl"
#define PACKCL_PATH "C:\\Program Files\\Common Files\\OFX\\Plugins\\PointsPlugin.ofx.bundle\\Contents\\Resources\\PointsPluginCLKernel.pk"
