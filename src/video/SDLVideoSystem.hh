// $Id$

#ifndef SDLVIDEOSYSTEM_HH
#define SDLVIDEOSYSTEM_HH

#include "VideoSystem.hh"
#include "RendererFactory.hh"

struct SDL_Surface;

namespace openmsx {

class VDP;
class Rasterizer;
class V9990;
class V9990Rasterizer;

class SDLVideoSystem: public VideoSystem
{
public:
	SDLVideoSystem();
	virtual ~SDLVideoSystem();

	// VideoSystem interface:
	virtual Rasterizer* createRasterizer(VDP& vdp);
	virtual V9990Rasterizer* createV9990Rasterizer(V9990& vdp);
	virtual bool checkSettings();
	virtual bool prepare();
	virtual void flush();
	virtual void takeScreenShot(const std::string& filename);

private:
	SDL_Surface* screen;
};

} // namespace openmsx

#endif
