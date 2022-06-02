/*************************************************************************\
*
*  Power48.h
*
*  Main header file
*
*  (c)2002 by Robert Hildinger
*
\*************************************************************************/

#include <PalmOS.h>
#include <PceNativeCall.h>

// Event Codes
#define volumeUnmountedEvent 				0x6101
#define volumeMountedEvent 					0x6102
#define invokeColorSelectorEvent			0x6103
#define virtualPenDownEvent					0x6104
#define virtualPenUpEvent					0x6105

#define vchrSliderClose 	1284
#define vchrSliderOpen  	1285

#define errOnSimulator	0x8009


//#define COMPILE_BATTERY_TEST


#define ByteSwap16(n) ( ((((unsigned int) n) << 8) & 0xFF00) | \
                        ((((unsigned int) n) >> 8) & 0x00FF) )

#define ByteSwap32(n) ( ((((unsigned long) n) << 24) & 0xFF000000) |	\
                        ((((unsigned long) n) <<  8) & 0x00FF0000) |	\
                        ((((unsigned long) n) >>  8) & 0x0000FF00) |	\
                        ((((unsigned long) n) >> 24) & 0x000000FF) )


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
 
#define StateFileType			'staF'
#define RomFileType				'romF'
#define RamFileType				'ramF'
#define ObjFileType				'objF'
#define sxStateFileName			"Power48 48SX STATE"
#define gxStateFileName			"Power48 48GX STATE"
#define g49StateFileName		"Power48 49G STATE"
#define sxINTROMFileName		"Power48 48SX ROM"
#define sxINTRAMFileName		"Power48 48SX RAM"
#define sxINTPORT1FileName		"Power48 48SX PORT1"
#define gxINTROMFileName		"Power48 48GX ROM"
#define gxINTRAMFileName		"Power48 48GX RAM"
#define gxINTPORT1FileName		"Power48 48GX PORT1"
#define g49INTROMFileName		"Power48 49G ROM"
#define g49INTIRAMFileName		"Power48 49G IRAM"
#define g49INTERAMFileName		"Power48 49G ERAM"

#define ROMFileExtension		".p48rom"
#define RAMFileExtension		".p48ram"
#define OBJFileExtension		".p48obj"
#define VFSROMRAMDirectory		"/PALM/PROGRAMS/Power48/"
#define objectVFSDirName		"/PALM/PROGRAMS/Power48/Objects/"
#define sxVFSROMFileName		"/PALM/PROGRAMS/Power48/HP48SX.p48rom"
#define sxVFSRAMFileName		"/PALM/PROGRAMS/Power48/HP48SXRam.p48ram"
#define sxVFSPORT1FileName		"/PALM/PROGRAMS/Power48/HP48SXPort1.p48ram"
#define gxVFSROMFileName		"/PALM/PROGRAMS/Power48/HP48GX.p48rom"
#define gxVFSRAMFileName		"/PALM/PROGRAMS/Power48/HP48GXRam.p48ram"
#define gxVFSPORT1FileName		"/PALM/PROGRAMS/Power48/HP48GXPort1.p48ram"
#define g49VFSROMFileName		"/PALM/PROGRAMS/Power48/HP49G.p48rom"
#define g49VFSIRAMFileName		"/PALM/PROGRAMS/Power48/HP49GIRam.p48ram"
#define g49VFSERAMFileName		"/PALM/PROGRAMS/Power48/HP49GERam.p48ram"

#define sxVFSBackImage2x2Name		"/PALM/PROGRAMS/Power48/HP48SXBackImage2x2.bmp"
#define sxVFSBackImage1x1Name		"/PALM/PROGRAMS/Power48/HP48SXBackImage1x1.bmp"
#define gxVFSBackImage2x2Name		"/PALM/PROGRAMS/Power48/HP48GXBackImage2x2.bmp"
#define gxVFSBackImage1x1Name		"/PALM/PROGRAMS/Power48/HP48GXBackImage1x1.bmp"
#define g49VFSBackImage2x2Name		"/PALM/PROGRAMS/Power48/HP49GBackImage2x2.bmp"
#define g49VFSBackImage1x1Name		"/PALM/PROGRAMS/Power48/HP49GBackImage1x1.bmp"

#define romMemSXFtrNum 			100
#define romMemGXFtrNum 			101
#define romMem49FtrNum			102
#define ramMemSXFtrNum			103
#define ramMemGXFtrNum 			104
#define ramMem49FtrNum			105

#define tempObjMemFtrNum 		200
#define tempBMPFtrNum 			201

#define STRUCTURE_HEADER		'P48S'
#define STRUCTURE_VERSION		1

/* Define the minimum OS version we support */
#define ourMinVersion    sysMakeROMVersion(5,0,0,sysROMStageDevelopment,0)
#define kPalmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)

// Error Codes
#define errUnableToAllocateRAM				0x8000
#define errUnableToLoadROMFile				0x8001
#define errUnableToLoadRAMFile				0x8002
#define errUnableToFindROMFiles				0x8003
#define errUnableToAllocateROM				0x8004




// noKey and byteswapped_noKey are now defined in Power48_core.h
//#define noKey				99
//#define byteswapped_noKey	0x6300

#define softKeyA			0
#define softKeyB			1
#define softKeyC			2
#define softKeyD			3
#define softKeyE			4
#define softKeyF			5

#define keyMTH				6
#define keyPRG				7
#define keyCST				8
#define keyVAR				9
#define keyUp				10
#define keyNXT				11

#define keyQuote			12
#define keySTO				13
#define keyEVAL				14
#define keyLEFT				15
#define keyDOWN				16
#define keyRIGHT			17

#define keySIN				18
#define keyCOS				19
#define keyTAN				20
#define keySquareRoot		21
#define keyYtotheX			22
#define key1OverX			23

#define keyENTER			24
#define keyPlusMinus		25
#define keyEEX				26
#define keyDEL				27
#define keyBackspace		28

#define keyAlpha			29
#define key7				30
#define key8				31
#define key9				32
#define keyDivide			33

#define keyLeftShift		34
#define key4				35
#define key5				36
#define key6				37
#define keyMultiply			38

#define keyRightShift		39
#define key1				40
#define key2				41
#define key3				42
#define keySubtract			43

#define keyON				44
#define key0				45
#define keyPeriod			46
#define keySPC				47
#define keyAdd				48

#define keyAPPS				49
#define keyMODE				50
#define keyTOOL				51
#define keyHIST				52
#define keyCAT				53
#define keyEQW				54
#define keySYMB				55
#define keyX				56

#define keyAbout48GX		57
#define keyOnPlus			58
#define keyObject			59
#define keySwitch			60

#define maxKeys				61



#define virtKeyBase				0x80

#define virtKeySoftKeyA			0x80
#define virtKeySoftKeyB			0x81
#define virtKeySoftKeyC			0x82
#define virtKeySoftKeyD			0x83
#define virtKeySoftKeyE			0x84
#define virtKeySoftKeyF			0x85
#define virtKeyLabelNxt			0x86
#define virtKeyLabelPrev		0x87

#define maxVirtKeys				8

#define virtKeyIPSMonitor		0x99
// virtKeyVideoSwap is now defined in Power48_core.h
//#define virtKeyVideoSwap		0xFF   // don't include in max




#define noPref						99

#define prefGeneral					0
#define pref48GX					1
#define pref48SX					2	
#define pref49G						3
#define prefCancel					4
#define prefOK						5
#define prefGenDisplayMonitor		6
#define prefGenTrueSpeed			7 
#define prefGenKeyClick				8 
#define prefGenVerbose				9
#define prefGenVerboseSlow			10
#define prefGenVerboseFast			11
#define prefGenReset				12
#define prefGenColorSelect			13
#define pref48GXBackdrop			14
#define pref48GXColorCycle			15	
#define pref48GX128KExpansion		16
#define pref48GXActive				17
#define pref48SXBackdrop			18
#define pref48SXColorCycle			19
#define pref48SX128KExpansion		20
#define pref48SXActive				21
#define pref49GBackdrop				22
#define pref49GColorCycle			23
#define pref49GActive				24

#define maxPrefButtons 				25



#define noObject			99

#define objectLoad 			0
#define objectSave 			1
#define objectCancel		2
#define objectOK			3
#define objectList			4
#define objectUp			5
#define objectDown			6
#define objectInternal		7
#define objectCard			8

#define maxObjectButtons 	9



#define noAlertConfirmButton	99

#define AlertConfirmNO			0
#define AlertConfirmYESorOK		1

#define maxAlertConfirmButtons 	2



#define noColorButton				99

#define colorCancel					0
#define colorDefault				1
#define colorOK						2	
#define colorBackRed				3
#define colorBackGreen				4
#define colorBackBlue				5
#define colorPixelRed				6	
#define colorPixelGreen				7
#define colorPixelBlue				8

#define maxColorButtons 			9




#define RAM_SIZE_GX 				0x40000
#define ROM_SIZE_GX 				0x100000
#define RAM_SIZE_SX 				0x10000
#define ROM_SIZE_SX 				0x080000
#define PORT1_SIZE					0x40000
#define IRAM_SIZE_49				0x080000
#define ERAM_SIZE_49				0x080000
#define ROM_SIZE_49					0x200000

#define TIME_OFFSET		0x0001bfa9d9700000



#ifndef __CALCTARGET__
#define __CALCTARGET__

typedef enum 		// Make sure any changes to calcTarget here
{					// are reflected in Power48.h
	HP48SX,
	HP48GX,
	HP49G
} calcTarget;

typedef enum
{
	KEY_RELEASE_ALL,
	KEY_PRESS_SINGLE
} KHType;

#endif
 
typedef enum
{
	ROM_UNAVAILABLE=0,
	ROM_INTERNAL,
	ROM_CARD
} RomLocation;

typedef enum
{
	ALL_IN_FTRMEM=0,
	ROM_IN_FTRMEM,
	ALL_IN_DHEAP
} MemLocation;


typedef enum
{
	COMPRESS,
	UNCOMPRESS,
	CHECKSUM
} compOpType;

typedef enum
{
	SCROLL_UP,
	SCROLL_DOWN,
	REDRAW
} RPLactioncode;

typedef enum
{
	MHZ,
	MIPS
} PanelType;

typedef enum
{
	TYPE_ONE,
	TYPE_TWO
} effectTypeEnum;

typedef enum
{
	PALMONE,
	TAPWAVE
} NavType;


typedef struct {
	UInt16		*fromBasePtr;
	UInt16		*toBasePtr;
	UInt16		*destBasePtr;
	UInt32		destRowInt16s;
	UInt32		offsetX;
	UInt32		offsetY;
	UInt32		toWidth;
	UInt32		toHeight;
	UInt32		effectType;
	Int32		fadeOffset;
	UInt32		position;  // don't byteswap this guy
} drawInfo;

typedef struct {
	Char*		name;
	UInt16 		size;
	Boolean		isInternal;
	UInt16		cardNo;
	LocalID		dbID;
} objFileInfoType;


extern MemPtr corePointer;




typedef enum
{
	alert=1,
	confirm
} dialogType;

typedef enum
{
	STARTUP,
	SWITCH,
	SHUTDOWN,
	ANY
} ProgBarType; 


extern calcTarget  	optionTarget;
extern Boolean 		optionVerbose;
extern Boolean 		optionScrollFast;
extern Boolean 		optionThreeTwentySquared;
extern Boolean	 	option48GXBackdrop;
extern Boolean	 	option48SXBackdrop;
extern Boolean	 	option49GBackdrop;
extern Boolean	 	option48GXColorCycle;
extern Boolean	 	option48SXColorCycle;
extern Boolean	 	option49GColorCycle;
extern Boolean	 	option48GX128KExpansion;
extern Boolean	 	option48SX128KExpansion;
extern RGBColorType	option48GXPixelColor;
extern RGBColorType	option48GXBackColor;
extern RGBColorType	option48SXPixelColor;
extern RGBColorType	option48SXBackColor;
extern RGBColorType	option49GPixelColor;
extern RGBColorType	option49GBackColor;
extern PanelType	optionPanelType;
extern Int16		optionPanelX;
extern Int16		optionPanelY;
extern Boolean		optionPanelOpen;
extern Boolean		optionTrueSpeed;
extern Boolean 		optionDisplayZoomed;
extern Boolean		optionDisplayMonitor;
extern Boolean		optionKeyClick;

extern Boolean previousOptionThreeTwentySquared;

typedef struct {
	Boolean 	verbose;
	Boolean 	scrollFast;
	Boolean 	oThreeTwentySquared;
	Boolean	 	o48GXBackdrop;
	
	Boolean	 	o48SXBackdrop;
	Boolean	 	o49GBackdrop;
	Boolean	 	o48GXColorCycle;
	Boolean	 	o48SXColorCycle;
	
	Boolean	 	o49GColorCycle;
	Boolean	 	o48GX128KExpansion;
	Boolean	 	o48SX128KExpansion;
	Boolean		oDisplayZoomed;
	
	Boolean		oDisplayMonitor;
	Boolean		oTrueSpeed;
	Boolean		oPanelOpen;
	Boolean		oKeyClick;
	
	PanelType	oPanelType;
	calcTarget	target;
	Int16		oPanelX;
	Int16		oPanelY;
	
	RGBColorType	o48GXPixelColor;
	RGBColorType	o48GXBackColor;
	RGBColorType	o48SXPixelColor;
	RGBColorType	o48SXBackColor;
	RGBColorType	o49GPixelColor;
	RGBColorType	o49GBackColor;
} pref_t;

typedef struct {
	UInt16		volume;
	Boolean 	loadSX;
	Boolean 	powerSave;
	Boolean 	verbose;
	Boolean 	scrollFast;
	Boolean 	oThreeTwentySquared;
	Boolean	 	o48GXBackdrop;
	Boolean	 	o48SXBackdrop;
	Boolean	 	o49GBackdrop;
	Boolean	 	o48GXColorCycle;
	Boolean	 	o48SXColorCycle;
	Boolean	 	o49GColorCycle;
	Boolean	 	o48GX128KExpansion;
	Boolean	 	o48SX128KExpansion;
	calcTarget	target;
	RGBColorType	o48GXPixelColor;
	RGBColorType	o48GXBackColor;
	RGBColorType	o48SXPixelColor;
	RGBColorType	o48SXBackColor;
	RGBColorType	o49GPixelColor;
	RGBColorType	o49GBackColor;
} pref_t_100;

typedef struct {
	UInt16		volume;
	Boolean 	loadSX;
	Boolean 	powerSave;
	Boolean 	verbose;
	Boolean 	scrollFast;
	Boolean 	turbo;
	Boolean 	oThreeTwentySquared;
	Boolean	 	o48GXColorForContrast;
	Boolean	 	o48SXColorForContrast;
	Boolean	 	o48GXWhiteBackground;
	Boolean	 	o48SXWhiteBackground;
	Boolean	 	o48GX128KExpansion;
	Boolean	 	o48SX128KExpansion;
} pref_t_090b;                     // version 0.90 beta pref format

extern Boolean 		statusCompleteReset;
extern Boolean 		statusSwitchEmulation;
extern Boolean 		status128KExpansion;

extern Boolean		statusBackdrop;
extern Boolean 		statusColorCycle;
extern Boolean 		statusPortraitViaLandscape;

extern RomLocation	status48SXROMLocation;
extern RomLocation	status48GXROMLocation;
extern RomLocation	status49GROMLocation;
extern Boolean		statusInternalFiles;
extern Boolean		statusVolumeMounted;
extern Boolean		statusInUnmountDialog;
extern Boolean		statusRUN;
extern Boolean		statusSkipScreenDraw;
extern UInt8		statusPrefTabActive;
extern UInt8		statusObjectLSActive;
extern calcTarget 	statusNextTarget;
extern RGBColorType	*statusPixelColor;
extern RGBColorType	*statusBackColor;
extern MemLocation	statusUseSystemHeap;
extern Boolean		statusLandscapeAvailable;
extern Boolean		statusCoreLoaded;
extern Boolean		statusPanelHidden;
extern Boolean 		statusDisplayMode2x2;
extern Boolean 		status5WayNavPresent;
extern NavType 		status5WayNavType;


extern UInt8 *romBase;  // pure, unswapped pointer to allocated ROM memory space
extern UInt8 *ramBase;  // pure, unswapped pointer to allocated RAM memory space
extern UInt8 *port1Base;  // pure, unswapped pointer to allocated PORT1 memory space
extern UInt8 *port2Base;  // pure, unswapped pointer to allocated PORT2 memory space

extern UInt32 magicNumberState;
extern UInt32 magicNumberRAM;
extern UInt32 magicNumberPORT;
extern UInt32 newMagicNumber;


extern UInt32 emulate(void);

extern void init_lcd();
extern void exit_lcd();
extern void display_clear(void);
extern void kbd_handler(UInt16 keyIndex, KHType action);
extern void rect_sweep(RectangleType *oldRect,RectangleType *newRect, int numSteps);
extern void toggle_lcd();
extern void ResetTime(void);
extern void AdjustTime(void);

extern UInt16 msVolRefNum;
extern UInt16 silkLibRefNum;
extern Boolean collapsibleIA;
extern UInt32 PINVersion;
extern UInt32 SILKVersion;

extern WinHandle mainDisplayWindow;
extern WinHandle backStoreWindow;
extern WinHandle annunciatorBitmapWindow;	
extern WinHandle backdrop1x1;
extern WinHandle backdrop2x2;
extern WinHandle backdrop2x2PVL;
extern WinHandle checkBoxWindow;	
extern WinHandle greyBackStore;
extern WinHandle panelWindow;
extern WinHandle digitsWindow;
extern RGBColorType backRGB,textRGB;

extern UInt32 screenHeight;
extern UInt32 screenRowInt16s;
extern Int16 displayExtX;
extern Int16 displayExtY;

extern const RGBColorType redRGB;
extern const RGBColorType greenRGB;
extern const RGBColorType blueRGB;
extern const RGBColorType whiteRGB;
extern const RGBColorType blackRGB;
extern const RGBColorType greyRGB;
extern const RGBColorType panelRGB;
extern const RGBColorType lightBlueRGB;
extern const RGBColorType lightGreenRGB;

#define maxExpandedTextWidth	320
#define maxExpandedTextHeight	32

#define lcdOriginX			46
#define lcdOriginY			15
#define fullScreenOriginX   46
#define fullScreenOriginY   1
#define fullScreenExtentX   262
#define fullScreenExtentY   142
#define lcd1x1OriginX		169
#define lcd1x1OriginY		3
#define lcd1x1FSOriginX		167
#define lcd1x1FSOriginY		3
#define lcd1x1FSExtentX		133
#define lcd1x1FSExtentY		64
#define lcd1x1AnnOriginX	153
#define lcd1x1AnnOriginY	3
#define lcd1x1AnnExtentX	12
#define lcd1x1AnnExtentY	64
#define lcd1x1DividerOriginX	165
#define lcd1x1DividerOriginY	3
#define lcd1x1DividerExtentX	2
#define lcd1x1DividerExtentY	64
#define lcd1x1TotalExtentX	148
#define lcd1x1TotalExtentY	64

#define lcdRows 			64
#define lcdColumns			131
#define displayRows			lcdRows*2
#define displayColumns		lcdColumns*2
#define screenPrintOriginX	lcdOriginX+2
#define screenPrintOriginY	lcdOriginY-13
#define screenPrintExtentX	displayColumns-4
#define screenPrintExtentY	displayRows+13
#define errorPrintOriginX	lcdOriginX+2
#define errorPrintOriginY	210
#define errorPrintExtentX	displayColumns
#define errorPrintExtentY	230
#define errorPrint320ExtentY	100
#define lineDiff			11
#define prefDialogOriginX	19
#define prefDialogOriginY	13
#define prefDialogExtentX	282
#define prefDialogExtentY	294
#define miniOffsetX			8
#define miniOffsetY			70
#define miniHeight			141
#define miniWidth			100
#define noImageOffsetX		18
#define noImageOffsetY		125
#define colorDialogOriginX	23
#define colorDialogOriginY	160
#define colorDialogExtentX	274
#define colorDialogExtentY	154
#define alertConfirmDialogOriginX	23
#define alertConfirmDialogOriginY	67
#define alertConfirmDialogExtentX	274
#define alertConfirmDialogExtentY	188
#define acDialogTextOriginX		alertConfirmDialogOriginX+8
#define acDialogTextOriginY		alertConfirmDialogOriginY+36
#define acDialogTextWidth		alertConfirmDialogExtentX-14
#define objectLSDialogOriginX	19
#define objectLSDialogOriginY	19
#define objectLSDialogExtentX	282
#define objectLSDialogExtentY	262
#define objectLSSaveFieldOriginX 	objectLSDialogOriginX+34
#define objectLSSaveFieldOriginY 	objectLSDialogOriginY+172
#define objectLSSaveFieldExtentX 	216
#define objectLSSaveFieldExtentY 	30

#define digitsTotalWidth		82
#define digitsWidth				8
#define digitsHeight			14
#define digitsPeriodWidth		2
#define digitsSpace				1
#define digitsOffsetX			panelTabWidth+7
#define digitsOffsetY			10
#define panelWidth				72
#define panelHeight				27
#define panelTabWidth			12
#define MAX_PANELS				2
#define SNAP_WIDTH				8
#define TICKS_PER_UPDATE		100

extern UInt32 lastInstCount;
extern UInt32 lastCycleCount;


typedef struct
{
	UInt16		upperLeftX;
	UInt16		upperLeftY;
	UInt16		lowerRightX;
	UInt16		lowerRightY;
	UInt8		row;
	UInt8		mask;
} keyDef;

typedef struct
{
	UInt16		upperLeftX;
	UInt16		upperLeftY;
	UInt16		lowerRightX;
	UInt16		lowerRightY;
} squareDef;

typedef struct
{
	UInt16		upperLeftX;
	UInt16		upperLeftY;
	UInt16		lowerRightX;
	UInt16		lowerRightY;
	UInt8		scope;
} prefButton;

typedef struct
{
	UInt16		upperLeftX;
	UInt16		upperLeftY;
	UInt16		lowerRightX;
	UInt16		lowerRightY;
	UInt8		scope;
} objectButton;

typedef struct
{
	Int16		deltaX;
	Int16		deltaY;
} point;

typedef enum
{
	SINGLE_DENSITY,
	DOUBLE_DENSITY
} densType;


extern const 	keyDef 	keyLayout450[];
extern const 	keyDef 	keyLayout450_hp49[];
extern const 	keyDef 	keyLayout320[];
extern const 	keyDef 	keyLayout320_hp49[];
extern keyDef*	keyLayout;

extern const 	squareDef virtLayout2x2[];
extern const 	squareDef virtLayout1x1[];
extern squareDef*	virtLayout;

extern Err 		MemPtrNewLarge(Int32 size, void **newPtr);
extern WinHandle OverwriteWindow(Coord width, Coord height, WindowFormatType format, UInt16 *error, WinHandle currWin);
extern void 	swapRectPVL(RectangleType *r,UInt16 height);
extern void 	FastDrawBitmap(BitmapPtr sourceBitmap, Coord x, Coord y);
extern void		LoadBackgroundImage(calcTarget loadTarget);
extern Boolean 	CheckDisplayTypeForGUIRedraw(void);
extern Err		DisplayHP48Screen(calcTarget loadTarget);
extern void		GUIRedraw(void);
extern void 	CrossFade(WinHandle srcWin, WinHandle destWin, UInt32 offsetX, UInt32 offsetY, Boolean transparent,UInt16 stepping);
extern UInt32 	ByteUtility(UInt8* buf, UInt32 tSize, compOpType operation);
extern void 	DrawCharsExpanded (char *text, UInt16 length, Int16 x, Int16 y, Int16 maxWidth, WinDrawOperation operation, densType density);
extern void 	ErrorPrint( char* screenText, Boolean newLine );
extern void 	ScreenPrint( char* screenText, Boolean newLine );
extern void 	ScreenPrintReset( );
extern Boolean 	SaveRAM(Boolean skipToPort1);
extern UInt16 	TranslateEventToKey(EventPtr event);
extern void		CheckForRemount(void);
extern Boolean 	ProcessPalmEvent(UInt32 timeout);
extern void		DepressVirtualKey(UInt16 keyCode);
extern void		ReleaseVirtualKey(UInt16 keyCode);
extern void		LockVirtualKey(UInt16 keyCode);
extern void		DisplayAboutScreen(void);
extern void		DisplayPreferenceScreen(void);
extern void 	ColorSelectorScreen(void);
extern void		DisplaySwitchGraphic(void);
extern void		PinResetEmulator(void);
extern void		OpenObjectLoadSaveDialog(void);
extern Int8 	OpenAlertConfirmDialog(dialogType alertOrConfirm,DmResID stringID,Boolean skipSave);
extern void 	simulate_keypress(UInt16 keyIndex);
extern void 	GreyOutRectangle(RectangleType opRect, WinHandle win);
extern void 	BmpExtractor24(UInt8 *buf, UInt32 bufSize, WinHandle win);
extern void 	HandlePanelPress(Coord penX, Coord penY);
extern void 	UpdatePanelDisplay(void);
extern void 	InitializePanelDisplay(Boolean total);
extern void 	HidePanel(void);
extern void 	ShowPanel(void);
extern void		SetCPUSpeed(void);
extern void 	P48WinCopyRectangle(WinHandle srcWin, WinHandle dstWin, RectangleType *srcRect, Coord destX, Coord destY, WinDrawOperation mode);
extern void 	P48WinDrawRectangle(RectangleType *rP, UInt16 cornerDiam);
extern void 	P48WinPaintRectangle(RectangleType *rP, UInt16 cornerDiam);
extern void 	P48WinEraseRectangle(RectangleType *rP, UInt16 cornerDiam);
extern UInt8* 	GetRealAddressNoIO(UInt32 d);


#define appFileCreator			'HILD'	// register your own at http://www.palmos.com/dev/creatorid/
#define appName                 "Power48"
#define appVersionNum			"1.5.1"
#define appPrefID				0x01
#define appPrefVersionNum		0x07
#define appPrefVersionNum_100	0x05
#define appPrefVersionNum_090b	0x02
