// $Id$

/*
TODO:
- Idea: make an abstract superclass for line-based Renderers, this
  class would know when to sync etc, but not be SDL dependent.
  Since most of the abstraction is done using <Pixel>, most code
  is SDL independent already.
- Implement sprite pixels in Graphic 5.
- Is it possible to combine dirtyPattern and dirtyColour into a single
  dirty array?
  Pitfalls:
  * in SCREEN1, a colour change invalidates 8 consequetive characters
  * A12 and A11 of patternMask and colourMask may be different
    also, colourMask has A10..A6 as well
    in most realistic cases however the two will be of equal size
- Fix character mode dirty checking to work with incremental rendering.
  Approach 1:
  * use two dirty arrays, one for this frame, one for next frame
  * on every change, mark dirty in both arrays
    (checking line is useless because of vertical scroll on screen splits)
  * in frameStart, swap arrays
  Approach 2:
  * cache characters as 16x16 blocks and blit them to the screen
  * on a name change, do nothing
  * on a pattern or colour change, mark the block as dirty
  * if a to-be-blitted block is dirty, recalculate it
  I'll implement approach 1 on account of being very similar to the
  existing code. Some time I'll implement approach 2 as well and see
  if it is an improvement (in clarity and performance).
- Register dirty checker with VDP.
  This saves one virtual method call on every VRAM write. (does it?)
  Put some generic dirty check classes in Renderer.hh/cc.
*/

#include "SDLGLRenderer.hh"
#ifdef __SDLGLRENDERER_AVAILABLE__

#include "VDP.hh"
#include "VDPVRAM.hh"
#include "SpriteChecker.hh"
#include "RealTime.hh"
//#include "ConsoleSource/SDLConsole.hh"
#include <math.h>

/** Dimensions of screen.
  */
static const int WIDTH = 640;
static const int HEIGHT = 480;

/** Line number where top border starts.
  * This is independent of PAL/NTSC timing or number of lines per screen.
  */
static const int LINE_TOP_BORDER = 3 + 13;

inline static void GLSetColour(SDLGLRenderer::Pixel colour)
{
	glColor3ub(colour & 0xFF, (colour >> 8) & 0xFF, (colour >> 16) & 0xFF);
}

inline static void GLBlitLine(
	SDLGLRenderer::Pixel *line, int n, int x, int y)
{
	// Set pixel format.
	glPixelStorei(GL_UNPACK_ROW_LENGTH, n);
	//glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
	glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);

	// Draw pixels in frame buffer.
	glRasterPos2i(x, y + 2);
	glDrawPixels(n, 1, GL_RGBA, GL_UNSIGNED_BYTE, line);
}

/** Fill a boolean array with a single value.
  * Optimised for byte-sized booleans,
  * but correct for every size.
  */
inline static void fillBool(bool *ptr, bool value, int nr)
{
#if SIZEOF_BOOL == 1
	memset(ptr, value, nr);
#else
	while (nr--) *ptr++ = value;
#endif
}

inline void SDLGLRenderer::setDisplayMode(int mode)
{
	dirtyChecker = modeToDirtyChecker[mode];
	if (vdp->isBitmapMode(mode)) {
		bitmapConverter.setDisplayMode(mode);
	} else {
		characterConverter.setDisplayMode(mode);
	}
	lineWidth = (mode == 0x09 || mode == 0x10 || mode == 0x14 ? 512 : 256);
	palSprites = (mode == 0x1C ? palGraphic7Sprites : palBg);
}

inline void SDLGLRenderer::renderUntil(const EmuTime &time)
{
	// TODO: Also calculate position within a line.
	int limit =
		(vdp->getTicksThisFrame(time) + VDP::TICKS_PER_LINE - 400)
		/ VDP::TICKS_PER_LINE;
	assert(limit <= (vdp->isPalTiming() ? 313 : 262));
	if (nextLine < limit) {
		(this->*phaseHandler)(nextLine, limit);
		nextLine = limit ;
	}
}

inline void SDLGLRenderer::sync(const EmuTime &time)
{
	vram->sync(time);
	renderUntil(time);
}

inline int SDLGLRenderer::getLeftBorder()
{
	return (WIDTH - 512) / 2 - 14 + vdp->getHorizontalAdjust() * 2
		+ (vdp->isTextMode() ? 18 : 0);
}

inline int SDLGLRenderer::getDisplayWidth()
{
	return vdp->isTextMode() ? 480 : 512;
}

inline SDLGLRenderer::Pixel *SDLGLRenderer::getLinePtr(
	Pixel *displayCache, int line)
{
	return displayCache + line * 512;
}

// TODO: Cache this?
inline SDLGLRenderer::Pixel SDLGLRenderer::getBorderColour()
{
	// TODO: Used knowledge of V9938 to merge two 4-bit colours
	//       into a single 8 bit colour for SCREEN8.
	//       Keep doing that or make VDP handle SCREEN8 differently?
	return
		( vdp->getDisplayMode() == 0x1C
		? PALETTE256[
			vdp->getBackgroundColour() | (vdp->getForegroundColour() << 4) ]
		: palBg[ vdp->getDisplayMode() == 0x10
		       ? vdp->getBackgroundColour() & 3
		       : vdp->getBackgroundColour()
		       ]
		);
}

inline void SDLGLRenderer::renderBitmapLines(
	byte line, int count)
{
	int mode = vdp->getDisplayMode();
	// Which bits in the name mask determine the page?
	int pageMask = 0x200 | vdp->getEvenOddMask();
	while (count--) {
		// TODO: Optimise addr and line; too many connversions right now.
		int vramLine = (vdp->getNameMask() >> 7) & (pageMask | line);
		if (lineValidInMode[vramLine] != mode) {
			int addr = (vramLine << 7) & vdp->getNameMask();
			const byte *vramPtr = vram->readArea(addr, addr + 128);
			bitmapConverter.convertLine(
				getLinePtr(bitmapDisplayCache, vramLine), vramPtr );
			lineValidInMode[vramLine] = mode;
		}
		line++; // is a byte, so wraps at 256
	}
}

inline void SDLGLRenderer::renderPlanarBitmapLines(
	byte line, int count)
{
	int mode = vdp->getDisplayMode();
	// Which bits in the name mask determine the page?
	int pageMask = vdp->getEvenOddMask();
	while (count--) {
		// TODO: Optimise addr and line; too many connversions right now.
		int vramLine = (vdp->getNameMask() >> 7) & (pageMask | line);
		if ( lineValidInMode[vramLine] != mode
		|| lineValidInMode[vramLine | 512] != mode ) {
			int addr0 = (vramLine << 7) & vdp->getNameMask();
			int addr1 = addr0 | 0x10000;
			const byte *vramPtr0 = vram->readArea(addr0, addr0 + 128);
			const byte *vramPtr1 = vram->readArea(addr1, addr1 + 128);
			bitmapConverter.convertLinePlanar(
				getLinePtr(bitmapDisplayCache, vramLine),
				vramPtr0, vramPtr1 );
			lineValidInMode[vramLine] =
				lineValidInMode[vramLine | 512] = mode;
		}
		line++; // is a byte, so wraps at 256
	}
}

inline void SDLGLRenderer::renderCharacterLines(
	byte line, int count)
{
	while (count--) {
		// Render this line.
		characterConverter.convertLine(
			getLinePtr(charDisplayCache, line), line);
		line++; // is a byte, so wraps at 256
	}
}

SDLGLRenderer::DirtyChecker
	// Use checkDirtyBitmap for every mode for which isBitmapMode is true.
	SDLGLRenderer::modeToDirtyChecker[] = {
		// M5 M4 = 0 0  (MSX1 modes)
		&SDLGLRenderer::checkDirtyMSX1, // Graphic 1
		&SDLGLRenderer::checkDirtyMSX1, // Text 1
		&SDLGLRenderer::checkDirtyMSX1, // Multicolour
		&SDLGLRenderer::checkDirtyNull,
		&SDLGLRenderer::checkDirtyMSX1, // Graphic 2
		&SDLGLRenderer::checkDirtyMSX1, // Text 1 Q
		&SDLGLRenderer::checkDirtyMSX1, // Multicolour Q
		&SDLGLRenderer::checkDirtyNull,
		// M5 M4 = 0 1
		&SDLGLRenderer::checkDirtyMSX1, // Graphic 3
		&SDLGLRenderer::checkDirtyText2,
		&SDLGLRenderer::checkDirtyNull,
		&SDLGLRenderer::checkDirtyNull,
		&SDLGLRenderer::checkDirtyBitmap, // Graphic 4
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		// M5 M4 = 1 0
		&SDLGLRenderer::checkDirtyBitmap, // Graphic 5
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap, // Graphic 6
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		// M5 M4 = 1 1
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap, // Graphic 7
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap,
		&SDLGLRenderer::checkDirtyBitmap
	};

SDLGLRenderer::SDLGLRenderer(
	VDP *vdp, SDL_Surface *screen, bool fullScreen, const EmuTime &time)
	: Renderer(fullScreen)
	, characterConverter(vdp, palFg, palBg)
	, bitmapConverter(palFg, PALETTE256)
{
	this->vdp = vdp;
	this->screen = screen;
	vram = vdp->getVRAM();
	spriteChecker = vdp->getSpriteChecker();
	// TODO: Store current time.
	//       Does the renderer actually have to keep time?
	//       Keeping render position should be good enough.

	// Init OpenGL settings.
	glViewport(0, 0, WIDTH, HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Init renderer state.
	phaseHandler = &SDLGLRenderer::blankPhase;
	setDisplayMode(vdp->getDisplayMode());
	setDirty(true);
	dirtyForeground = dirtyBackground = true;

	// Create display caches.
	charDisplayCache = new Pixel[(vdp->isMSX1VDP() ? 192 : 256) * 512];
	bitmapDisplayCache =
		vdp->isMSX1VDP() ? NULL : new Pixel[256 * 4 * 512];
	memset(lineValidInMode, 0xFF, sizeof(lineValidInMode));

	// Hide mouse cursor.
	SDL_ShowCursor(SDL_DISABLE);

	// Init the palette.
	if (vdp->isMSX1VDP()) {
		// Fixed palette.
		for (int i = 0; i < 16; i++) {
			palFg[i] = palBg[i] =
				   TMS99X8A_PALETTE[i][0]
				| (TMS99X8A_PALETTE[i][1] << 8)
				| (TMS99X8A_PALETTE[i][2] << 16)
				| 0xFF000000;
		}
	}
	else {
		// Precalculate SDL palette for V9938 colours.
		for (int r = 0; r < 8; r++) {
			for (int g = 0; g < 8; g++) {
				for (int b = 0; b < 8; b++) {
					const float gamma = 2.2 / 2.8;
					V9938_COLOURS[r][g][b] =
						   (int)(pow((float)r / 7.0, gamma) * 255)
						| ((int)(pow((float)g / 7.0, gamma) * 255) << 8)
						| ((int)(pow((float)b / 7.0, gamma) * 255) << 16)
						| 0xFF000000;
				}
			}
		}
		// Precalculate Graphic 7 bitmap palette.
		for (int i = 0; i < 256; i++) {
			PALETTE256[i] = V9938_COLOURS
				[(i & 0x1C) >> 2]
				[(i & 0xE0) >> 5]
				[((i & 0x03) << 1) | ((i & 0x02) >> 1)];
		}
		// Precalculate Graphic 7 sprite palette.
		for (int i = 0; i < 16; i++) {
			word grb = GRAPHIC7_SPRITE_PALETTE[i];
			palGraphic7Sprites[i] =
				V9938_COLOURS[(grb >> 4) & 7][grb >> 8][grb & 7];
		}
		// Reset the palette.
		for (int i = 0; i < 16; i++) {
			updatePalette(i, vdp->getPalette(i), time);
		}
	}

	// Now we're ready to start rendering the first frame.
	frameStart(time);
}

SDLGLRenderer::~SDLGLRenderer()
{
	delete charDisplayCache;
	delete bitmapDisplayCache;
}

void SDLGLRenderer::setFullScreen(
	bool fullScreen)
{
	Renderer::setFullScreen(fullScreen);
	if (((screen->flags & SDL_FULLSCREEN) != 0) != fullScreen) {
		SDL_WM_ToggleFullScreen(screen);
	}
}

void SDLGLRenderer::updateTransparency(
	bool enabled, const EmuTime &time)
{
	sync(time);
	// Set the right palette for pixels of colour 0.
	palFg[0] = palBg[enabled ? vdp->getBackgroundColour() : 0];
	// Any line containing pixels of colour 0 must be repainted.
	// We don't know which lines contain such pixels,
	// so we have to repaint them all.
	anyDirtyColour = true;
	fillBool(dirtyColour, true, sizeof(dirtyColour));
	memset(lineValidInMode, 0xFF, sizeof(lineValidInMode));
}

void SDLGLRenderer::updateForegroundColour(
	int colour, const EmuTime &time)
{
	sync(time);
	dirtyForeground = true;
}

void SDLGLRenderer::updateBackgroundColour(
	int colour, const EmuTime &time)
{
	sync(time);
	dirtyBackground = true;
	if (vdp->getTransparency()) {
		// Transparent pixels have background colour.
		palFg[0] = palBg[colour];
		// Any line containing pixels of colour 0 must be repainted.
		// We don't know which lines contain such pixels,
		// so we have to repaint them all.
		anyDirtyColour = true;
		fillBool(dirtyColour, true, sizeof(dirtyColour));
		memset(lineValidInMode, 0xFF, sizeof(lineValidInMode));
	}
}

void SDLGLRenderer::updateBlinkForegroundColour(
	int colour, const EmuTime &time)
{
	sync(time);
	dirtyForeground = true;
}

void SDLGLRenderer::updateBlinkBackgroundColour(
	int colour, const EmuTime &time)
{
	sync(time);
	dirtyBackground = true;
}

void SDLGLRenderer::updateBlinkState(
	bool enabled, const EmuTime &time)
{
	// TODO: When the sync call is enabled, the screen flashes on
	//       every call to this method.
	//       I don't know why exactly, but it's probably related to
	//       being called at frame start.
	//sync(time);
	if (vdp->getDisplayMode() == 0x09) {
		// Text2 with blinking text.
		// Consider all characters dirty.
		// TODO: Only mark characters in blink colour dirty.
		anyDirtyName = true;
		fillBool(dirtyName, true, sizeof(dirtyName));
	}
}

void SDLGLRenderer::updatePalette(
	int index, int grb, const EmuTime &time)
{
	sync(time);

	// Update SDL colours in palette.
	palFg[index] = palBg[index] =
		V9938_COLOURS[(grb >> 4) & 7][grb >> 8][grb & 7];

	// Is this the background colour?
	if (vdp->getBackgroundColour() == index && vdp->getTransparency()) {
		dirtyBackground = true;
		// Transparent pixels have background colour.
		palFg[0] = palBg[vdp->getBackgroundColour()];
	}

	// Any line containing pixels of this colour must be repainted.
	// We don't know which lines contain which colours,
	// so we have to repaint them all.
	anyDirtyColour = true;
	fillBool(dirtyColour, true, sizeof(dirtyColour));
	memset(lineValidInMode, 0xFF, sizeof(lineValidInMode));
}

void SDLGLRenderer::updateVerticalScroll(
	int scroll, const EmuTime &time)
{
	sync(time);
}

void SDLGLRenderer::updateHorizontalAdjust(
	int adjust, const EmuTime &time)
{
	sync(time);
}

void SDLGLRenderer::updateDisplayEnabled(
	bool enabled, const EmuTime &time)
{
	sync(time);
	phaseHandler = ( enabled
		? &SDLGLRenderer::displayPhase : &SDLGLRenderer::blankPhase );
}

void SDLGLRenderer::updateDisplayMode(
	int mode, const EmuTime &time)
{
	sync(time);
	setDisplayMode(mode);
	setDirty(true);
}

void SDLGLRenderer::updateNameBase(
	int addr, const EmuTime &time)
{
	sync(time);
	anyDirtyName = true;
	fillBool(dirtyName, true, sizeof(dirtyName));
}

void SDLGLRenderer::updatePatternBase(
	int addr, const EmuTime &time)
{
	sync(time);
	anyDirtyPattern = true;
	fillBool(dirtyPattern, true, sizeof(dirtyPattern));
}

void SDLGLRenderer::updateColourBase(
	int addr, const EmuTime &time)
{
	sync(time);
	anyDirtyColour = true;
	fillBool(dirtyColour, true, sizeof(dirtyColour));
}

void SDLGLRenderer::updateVRAM(
	int addr, byte data, const EmuTime &time)
{
	// TODO: Is it possible to get rid of this method?
	//       One method call is a considerable overhead since VRAM
	//       changes occur pretty often.
	//       For example, register dirty checker at caller.

	// If display is disabled, VRAM changes will not affect the
	// renderer output, therefore sync is not necessary.
	// TODO: Changes in invisible pages do not require sync either.
	//       Maybe this is a task for the dirty checker, because what is
	//       visible is display mode dependant.
	if (vdp->isDisplayEnabled()) renderUntil(time);

	(this->*dirtyChecker)(addr, data);
}

void SDLGLRenderer::checkDirtyNull(
	int addr, byte data)
{
	// Do nothing: this is a bogus mode whose display doesn't depend
	// on VRAM contents.
}

void SDLGLRenderer::checkDirtyMSX1(
	int addr, byte data)
{
	if ((addr | ~(-1 << 10)) == vdp->getNameMask()) {
		dirtyName[addr & ~(-1 << 10)] = anyDirtyName = true;
	}
	if ((addr | ~(-1 << 13)) == vdp->getColourMask()) {
		dirtyColour[(addr / 8) & ~(-1 << 10)] = anyDirtyColour = true;
	}
	if ((addr | ~(-1 << 13)) == vdp->getPatternMask()) {
		dirtyPattern[(addr / 8) & ~(-1 << 10)] = anyDirtyPattern = true;
	}
}

void SDLGLRenderer::checkDirtyText2(
	int addr, byte data)
{
	int nameBase = vdp->getNameMask() & (-1 << 12);
	int i = addr - nameBase;
	if ((0 <= i) && (i < 2160)) {
		dirtyName[i] = anyDirtyName = true;
	}
	if ((addr | ~(-1 << 11)) == vdp->getPatternMask()) {
		dirtyPattern[(addr / 8) & ~(-1 << 8)] = anyDirtyPattern = true;
	}
	// TODO: Implement dirty check on colour table (used for blinking).
}

void SDLGLRenderer::checkDirtyBitmap(
	int addr, byte data)
{
	lineValidInMode[addr >> 7] = 0xFF;
}

void SDLGLRenderer::setDirty(
	bool dirty)
{
	anyDirtyColour = anyDirtyPattern = anyDirtyName = dirty;
	fillBool(dirtyName, dirty, sizeof(dirtyName));
	fillBool(dirtyColour, dirty, sizeof(dirtyColour));
	fillBool(dirtyPattern, dirty, sizeof(dirtyPattern));
}

void SDLGLRenderer::drawSprites(
	int absLine)
{
	// Check whether this line is inside the host screen.
	int screenLine = (absLine - lineRenderTop) * 2;
	if (screenLine >= HEIGHT) return;

	// Determine sprites visible on this line.
	SpriteChecker::SpriteInfo *visibleSprites;
	int visibleIndex =
		spriteChecker->getSprites(absLine, visibleSprites);
	// Optimisation: return at once if no sprites on this line.
	// Lines without any sprites are very common in most programs.
	if (visibleIndex == 0) return;

	int leftBorder = getLeftBorder();
	glPixelZoom(2.0, 2.0);

	if (vdp->getDisplayMode() < 8) {
		// Sprite mode 1: render directly to screen using overdraw.
		while (visibleIndex--) {
			// Get sprite info.
			SpriteChecker::SpriteInfo *sip = &visibleSprites[visibleIndex];
			Pixel colour = sip->colourAttrib & 0x0F;
			// Don't draw transparent sprites in sprite mode 1.
			// TODO: Verify on real V9938 that sprite mode 1 indeed
			//       ignores the transparency bit.
			if (colour == 0) continue;
			colour = palSprites[colour];
			SpriteChecker::SpritePattern pattern = sip->pattern;
			int x = sip->x;
			// Skip any dots that end up in the border.
			if (x < 0) {
				pattern <<= -x;
				x = 0;
			} else if (x > 256 - 32) {
				pattern &= -1 << (32 - (256 - x));
			}
			// Convert pattern to pixels.
			Pixel buffer[32];
			Pixel *p = buffer;
			while (pattern) {
				// Draw pixel if sprite has a dot.
				*p++ = pattern & 0x80000000 ? colour : 0;
				// Advancing behaviour.
				pattern <<= 1;
			}
			int n = p - buffer;
			if (n) GLBlitLine(buffer, n, leftBorder + x * 2, screenLine);
		}
	} else {
		// Sprite mode 2: single pass left-to-right render.

		// Buffer to render sprite pixel to; start with all transparent.
		Pixel buffer[256];
		memset(buffer, 0, sizeof(buffer));
		// Determine width of sprites.
		SpriteChecker::SpritePattern combined = 0;
		for (int i = 0; i < visibleIndex; i++) {
			combined |= visibleSprites[i].pattern;
		}
		int size = 0;
		while (combined) {
			size++;
			combined <<= 1;
		}
		// Left-to-right scan.
		for (int pixelDone = 0; pixelDone < 256; pixelDone++) {
			// Skip pixels if possible.
			int minStart = pixelDone - size;
			int leftMost = 0xFFFF;
			for (int i = 0; i < visibleIndex; i++) {
				int x = visibleSprites[i].x;
				if (minStart < x && x < leftMost) leftMost = x;
			}
			if (leftMost > pixelDone) {
				pixelDone = leftMost;
				if (pixelDone >= 256) break;
			}
			// Calculate colour of pixel to be plotted.
			byte colour = 0xFF;
			for (int i = 0; i < visibleIndex; i++) {
				SpriteChecker::SpriteInfo *sip = &visibleSprites[i];
				int shift = pixelDone - sip->x;
				if ((0 <= shift && shift < 32)
				&& ((sip->pattern << shift) & 0x80000000)) {
					byte c = sip->colourAttrib & 0x0F;
					if (c == 0 && vdp->getTransparency()) continue;
					colour = c;
					// Merge in any following CC=1 sprites.
					for (i++ ; i < visibleIndex; i++) {
						sip = &visibleSprites[i];
						if (!(sip->colourAttrib & 0x40)) break;
						int shift = pixelDone - sip->x;
						if ((0 <= shift && shift < 32)
						&& ((sip->pattern << shift) & 0x80000000)) {
							colour |= sip->colourAttrib & 0x0F;
						}
					}
					break;
				}
			}
			// Plot it.
			if (colour != 0xFF) {
				buffer[pixelDone] = palSprites[colour];
			}
		}
		GLBlitLine(buffer, 256, leftBorder, screenLine);
	}
}

void SDLGLRenderer::blankPhase(
	int fromLine, int limit)
{
	// TODO: Only redraw if necessary.
	GLSetColour(getBorderColour());
	int y1 = (fromLine - lineRenderTop) * 2;
	int y2 = (limit - lineRenderTop) * 2;
	glBegin(GL_QUADS);
	glVertex2i(0, y1); // top left
	glVertex2i(WIDTH, y1); // top right
	glVertex2i(WIDTH, y2); // bottom right
	glVertex2i(0, y2); // bottom left
	glEnd();
}

void SDLGLRenderer::displayPhase(
	int fromLine, int limit)
{
	//cerr << "displayPhase from " << fromLine << " until " << limit << "\n";

	// Check for bottom erase; even on overscan this suspends display.
	if (limit > lineBottomErase) {
		limit = lineBottomErase;
	}
	if (limit > lineRenderTop + HEIGHT / 2) {
		limit = lineRenderTop + HEIGHT / 2;
	}
	if (fromLine >= limit) return;

	// GL render settings.
	glDisable(GL_BLEND);
	if (lineWidth == 512) {
		glPixelZoom(1.0, 2.0);
	} else {
		glPixelZoom(2.0, 2.0);
	}

	// Perform vertical scroll.
	int scrolledLine =
		(fromLine - vdp->getLineZero() + vdp->getVerticalScroll()) & 0xFF;

	// Render background lines.
	// TODO: Complete separation of character and bitmap modes.
	int leftBorder = getLeftBorder();
	int y = (fromLine - lineRenderTop) * 2;
	if (vdp->isBitmapMode()) {
		int line = scrolledLine;
		int n = limit - fromLine;
		bool planar = vdp->isPlanar();
		if (planar) renderPlanarBitmapLines(line, n);
		else renderBitmapLines(line, n);
		// Which bits in the name mask determine the page?
		int pageMask =
			(vdp->isPlanar() ? 0x000 : 0x200) | vdp->getEvenOddMask();
		do {
			int vramLine = (vdp->getNameMask() >> 7) & (pageMask | line);
			//Pixel lineBuffer[512];
			/*
			if ( (lineValidInMode[vramLine] != vdp->getDisplayMode())
			|| (planar && (lineValidInMode[vramLine | 512] != vdp->getDisplayMode())) ) {
				(this->*renderMethod)
					(getLinePtr(displayCache, vramLine), vramLine);
				lineValidInMode[vramLine] = vdp->getDisplayMode();
				if (planar) {
					lineValidInMode[vramLine | 512] = vdp->getDisplayMode();
				}
			}
			*/
			GLBlitLine(getLinePtr(bitmapDisplayCache, vramLine), lineWidth,
				leftBorder, y);
			//(this->*renderMethod)(lineBuffer, vramLine, leftBorder, y);
			line = (line + 1) & 0xFF;
			y += 2;
		} while (--n);
	} else {
		int line = scrolledLine;
		int n = limit - fromLine;
		renderCharacterLines(line, n);
		do {
			/*
			(this->*renderMethod)
				(getLinePtr(displayCache, line), line, leftBorder, y);
			*/
			GLBlitLine(getLinePtr(charDisplayCache, line), lineWidth,
				leftBorder, y);
			line = (line + 1) & 0xFF;
			y += 2;
		} while (--n);
	}

	/*
	// Copy background image.
	// TODO: Unify MSX1 and MSX2 modes?
	SDL_Rect source;
	source.x = 0;
	source.w = getDisplayWidth();
	source.h = 1;
	SDL_Rect dest;
	dest.x = getLeftBorder();
	dest.y = (fromLine - lineRenderTop) * 2;
	int line = scrolledLine;
	int pageMaskEven, pageMaskOdd;
	if (vdp->isInterlaced() && vdp->isEvenOddEnabled()) {
		pageMaskEven = vdp->isPlanar() ? 0x000 : 0x200;
		pageMaskOdd  = vdp->isPlanar() ? 0x100 : 0x300;
	} else {
		pageMaskEven = pageMaskOdd = pageMask;
	}
	// TODO: Optimise.
	for (int n = limit - fromLine; n--; ) {
		source.y =
			( vdp->isBitmapMode()
			? (vdp->getNameMask() >> 7) & (pageMaskEven | line)
			: line
			);
		// TODO: Can we safely use SDL_LowerBlit?
		// Note: return value is ignored.
		//SDL_BlitSurface(displayCache, &source, screen, &dest);
		glPixelZoom(1.0, 2.0);
		GLBlitLine(getLinePtr(displayCache, source.y), dest.x, dest.y);
		dest.y++;
		source.y =
			( vdp->isBitmapMode()
			? (vdp->getNameMask() >> 7) & (pageMaskOdd | line)
			: line
			);
		//SDL_BlitSurface(displayCache, &source, screen, &dest);
		//GLBlitLine(getLinePtr(displayCache, source.y), dest.x, dest.y);
		dest.y++;
		line = (line + 1) & 0xFF;
	}
	*/

	// Render sprites.
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.5f);
	for (int line = fromLine; line < limit; line++) {
		drawSprites(line);
	}
	glDisable(GL_ALPHA_TEST);

	// Borders are drawn after the display area:
	// V9958 can extend the left border over the display area,
	// this is implemented using overdraw.
	// TODO: Does the extended border clip sprites as well?
	GLSetColour(getBorderColour());
	int y1 = (fromLine - lineRenderTop) * 2;
	int y2 = (limit - lineRenderTop) * 2;
	glBegin(GL_QUADS);
	// Left border:
	int left = getLeftBorder();
	glVertex2i(0, y1); // top left
	glVertex2i(left, y1); // top right
	glVertex2i(left, y2); // bottom right
	glVertex2i(0, y2); // bottom left
	// Right border:
	int right = left + getDisplayWidth();
	glVertex2i(right, y1); // top left
	glVertex2i(WIDTH, y1); // top right
	glVertex2i(WIDTH, y2); // bottom right
	glVertex2i(right, y2); // bottom left
	glEnd();
}

void SDLGLRenderer::frameStart(const EmuTime &time)
{
	//cerr << "timing: " << (vdp->isPalTiming() ? "PAL" : "NTSC") << "\n";

	// Calculate line to render at top of screen.
	// Make sure the display area is centered.
	// 240 - 212 = 28 lines available for top/bottom border; 14 each.
	// NTSC: display at [32..244),
	// PAL:  display at [59..271).
	// TODO: Use screen lines instead.
	lineRenderTop = vdp->isPalTiming() ? 59 - 14 : 32 - 14;

	// Calculate important moments in frame rendering.
	lineBottomErase = vdp->isPalTiming() ? 313 - 3 : 262 - 3;
	nextLine = lineRenderTop;

	// Screen is up-to-date, so nothing is dirty.
	// TODO: Either adapt implementation to work with incremental
	//       rendering, or get rid of dirty tracking.
	//setDirty(false);
	//dirtyForeground = dirtyBackground = false;
}

void SDLGLRenderer::putImage(const EmuTime &time)
{
	// Render changes from this last frame.
	sync(time);

	// Render console if needed
	//Console::instance()->drawConsole();

	// Update screen.
	SDL_GL_SwapBuffers();

	// The screen will be locked for a while, so now is a good time
	// to perform real time sync.
	RealTime::instance()->sync();
}

Renderer *createSDLGLRenderer(VDP *vdp, bool fullScreen, const EmuTime &time)
{
	int flags = SDL_OPENGL | SDL_HWSURFACE
		| (fullScreen ? SDL_FULLSCREEN : 0);

	// Enables OpenGL double buffering.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

	// Try default bpp.
	SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, flags);

	// If no screen or unsupported screen,
	// try supported bpp in order of preference.
	int bytepp = (screen ? screen->format->BytesPerPixel : 0);
	if (bytepp != 1 && bytepp != 2 && bytepp != 4) {
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 15, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 16, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 8, flags);
	}

	if (!screen) {
		printf("FAILED to open any screen!");
		// TODO: Throw exception.
		return NULL;
	}
	PRT_DEBUG("Display is " << (int)(screen->format->BitsPerPixel) << " bpp.");
	//Trying to attach a console
	//SDLConsole::instance()->hookUpSDLConsole(screen);

	return new SDLGLRenderer(vdp, screen, fullScreen, time);
}

#endif // __SDLGLRENDERER_AVAILABLE__
