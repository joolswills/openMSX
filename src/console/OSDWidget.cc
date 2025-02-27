#include "OSDWidget.hh"
#include "OutputSurface.hh"
#include "Display.hh"
#include "CommandException.hh"
#include "TclObject.hh"
#include "GLUtil.hh"
#include "optional.hh"
#include "ranges.hh"
#include "stl.hh"
#include <SDL.h>
#include <array>
#include <limits>

using std::string;
using std::vector;
using std::unique_ptr;
using namespace gl;

namespace openmsx {

// intersect two rectangles
static void intersect(int xa, int ya, int wa, int ha,
                      int xb, int yb, int wb, int hb,
                      int& x, int& y, int& w, int& h)
{
	int x1 = std::max<int>(xa, xb);
	int y1 = std::max<int>(ya, yb);
	int x2 = std::min<int>(xa + wa, xb + wb);
	int y2 = std::min<int>(ya + ha, yb + hb);
	x = x1;
	y = y1;
	w = std::max(0, x2 - x1);
	h = std::max(0, y2 - y1);
}

////
template<typename T>
static void normalize(T& x, T& w)
{
	if (w < 0) {
		w = -w;
		x -= w;
	}
}

class SDLScopedClip
{
public:
	SDLScopedClip(OutputSurface& output, vec2 xy, vec2 wh);
	~SDLScopedClip();
private:
	SDL_Renderer* renderer;
	optional<SDL_Rect> origClip;
};


SDLScopedClip::SDLScopedClip(OutputSurface& output, vec2 xy, vec2 wh)
	: renderer(output.getSDLRenderer())
{
	ivec2 i_xy = round(xy); int x = i_xy[0]; int y = i_xy[1];
	ivec2 i_wh = round(wh); int w = i_wh[0]; int h = i_wh[1];
	normalize(x, w); normalize(y, h);

	int xn, yn, wn, hn;
	if (SDL_RenderIsClipEnabled(renderer)) {
		origClip.emplace();
		SDL_RenderGetClipRect(renderer, &*origClip);

		intersect(origClip->x, origClip->y, origClip->w, origClip->h,
			  x,  y,  w,  h,
			  xn, yn, wn, hn);
	} else {
		xn = x; yn = y; wn = w; hn = h;
	}
	SDL_Rect newClip = { Sint16(xn), Sint16(yn), Uint16(wn), Uint16(hn) };
	SDL_RenderSetClipRect(renderer, &newClip);
}

SDLScopedClip::~SDLScopedClip()
{
	SDL_RenderSetClipRect(renderer, origClip ? &*origClip : nullptr);
}

////

#if COMPONENT_GL

class GLScopedClip
{
public:
	GLScopedClip(OutputSurface& output, vec2 xy, vec2 wh);
	~GLScopedClip();
private:
	optional<std::array<GLint, 4>> origClip; // x, y, w, h;
};


GLScopedClip::GLScopedClip(OutputSurface& output, vec2 xy, vec2 wh)
{
	normalize(xy[0], wh[0]); normalize(xy[1], wh[1]);
	xy[1] = output.getHeight() - xy[1] - wh[1]; // openGL sets (0,0) in LOWER-left corner

	// transform view-space coordinates to clip-space coordinates
	vec2 scale = output.getViewScale();
	ivec2 i_xy = round(xy * scale) + output.getViewOffset();
	ivec2 i_wh = round(wh * scale);

	if (glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE) {
		origClip.emplace();
		glGetIntegerv(GL_SCISSOR_BOX, origClip->data());
		int xn, yn, wn, hn;
		intersect((*origClip)[0], (*origClip)[1], (*origClip)[2], (*origClip)[3],
		          i_xy[0], i_xy[1], i_wh[0], i_wh[1],
		          xn, yn, wn, hn);
		glScissor(xn, yn, wn, hn);
	} else {
		glScissor(i_xy[0], i_xy[1], i_wh[0], i_wh[1]);
		glEnable(GL_SCISSOR_TEST);
	}
}

GLScopedClip::~GLScopedClip()
{
	if (origClip) {
		glScissor((*origClip)[0], (*origClip)[1], (*origClip)[2], (*origClip)[3]);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
}

#endif

////

OSDWidget::OSDWidget(Display& display_, const TclObject& name_)
	: display(display_)
	, parent(nullptr)
	, name(name_)
	, z(0.0)
	, scaled(false)
	, clip(false)
	, suppressErrors(false)
{
}

void OSDWidget::addWidget(unique_ptr<OSDWidget> widget)
{
	widget->setParent(this);

	// Insert the new widget in the correct place (sorted on ascending Z)
	// heuristic: often we have either
	//  - many widgets with all the same Z
	//  - only a few total number of subwidgets (possibly with different Z)
	// In the former case we can simply append at the end. In the latter
	// case a linear search is probably faster than a binary search. Only
	// when there are many sub-widgets with not all the same Z (and not
	// created in sorted Z-order) a binary search would be faster.
	float widgetZ = widget->getZ();
	if (subWidgets.empty() || (subWidgets.back()->getZ() <= widgetZ)) {
		subWidgets.push_back(std::move(widget));
	} else {
		auto it = begin(subWidgets);
		while ((*it)->getZ() <= widgetZ) ++it;
		subWidgets.insert(it, std::move(widget));

	}
}

void OSDWidget::deleteWidget(OSDWidget& widget)
{
	auto it = rfind_if_unguarded(subWidgets,
		[&](const std::unique_ptr<OSDWidget>& p) { return p.get() == &widget; });
	subWidgets.erase(it);
}

#ifdef DEBUG
struct AscendingZ {
	bool operator()(const unique_ptr<OSDWidget>& lhs,
	                const unique_ptr<OSDWidget>& rhs) const {
		return lhs->getZ() < rhs->getZ();
	}
};
#endif
void OSDWidget::resortUp(OSDWidget* elem)
{
	// z-coordinate was increased, first search for elements current position
	auto it1 = begin(subWidgets);
	while (it1->get() != elem) ++it1;
	// next search for the position were it belongs
	float elemZ = elem->getZ();
	auto it2 = it1;
	++it2;
	while ((it2 != end(subWidgets)) && ((*it2)->getZ() < elemZ)) ++it2;
	// now move elements to correct position
	rotate(it1, it1 + 1, it2);
#ifdef DEBUG
	assert(ranges::is_sorted(subWidgets, AscendingZ()));
#endif
}
void OSDWidget::resortDown(OSDWidget* elem)
{
	// z-coordinate was decreased, first search for new position
	auto it1 = begin(subWidgets);
	float elemZ = elem->getZ();
	while ((*it1)->getZ() <= elemZ) {
		++it1;
		if (it1 == end(subWidgets)) return;
	}
	// next search for the elements current position
	auto it2 = it1;
	if ((it2 != begin(subWidgets)) && ((it2 - 1)->get() == elem)) return;
	while (it2->get() != elem) ++it2;
	// now move elements to correct position
	rotate(it1, it2, it2 + 1);
#ifdef DEBUG
	assert(ranges::is_sorted(subWidgets, AscendingZ()));
#endif
}

vector<string_view> OSDWidget::getProperties() const
{
	static const char* const vals[] = {
		"-type", "-x", "-y", "-z", "-relx", "-rely", "-scaled",
		"-clip", "-mousecoord", "-suppressErrors",
	};
	return to_vector<string_view>(vals);
}

void OSDWidget::setProperty(
	Interpreter& interp, string_view propName, const TclObject& value)
{
	if (propName == "-type") {
		throw CommandException("-type property is readonly");
	} else if (propName == "-mousecoord") {
		throw CommandException("-mousecoord property is readonly");
	} else if (propName == "-x") {
		pos[0] = value.getDouble(interp);
	} else if (propName == "-y") {
		pos[1] = value.getDouble(interp);
	} else if (propName == "-z") {
		float z2 = value.getDouble(interp);
		if (z != z2) {
			bool up = z2 > z; // was z increased?
			z = z2;
			if (auto* p = getParent()) {
				// TODO no need for a full sort: instead remove and re-insert in the correct place
				if (up) {
					p->resortUp(this);
				} else {
					p->resortDown(this);
				}
			}
		}
	} else if (propName == "-relx") {
		relPos[0] = value.getDouble(interp);
	} else if (propName == "-rely") {
		relPos[1] = value.getDouble(interp);
	} else if (propName == "-scaled") {
		bool scaled2 = value.getBoolean(interp);
		if (scaled != scaled2) {
			scaled = scaled2;
			invalidateRecursive();
		}
	} else if (propName == "-clip") {
		clip = value.getBoolean(interp);
	} else if (propName == "-suppressErrors") {
		suppressErrors = value.getBoolean(interp);
	} else {
		throw CommandException("No such property: ", propName);
	}
}

void OSDWidget::getProperty(string_view propName, TclObject& result) const
{
	if (propName == "-type") {
		result = getType();
	} else if (propName == "-x") {
		result = pos[0];
	} else if (propName == "-y") {
		result = pos[1];
	} else if (propName == "-z") {
		result = z;
	} else if (propName == "-relx") {
		result = relPos[0];
	} else if (propName == "-rely") {
		result = relPos[1];
	} else if (propName == "-scaled") {
		result = scaled;
	} else if (propName == "-clip") {
		result = clip;
	} else if (propName == "-mousecoord") {
		vec2 coord = getMouseCoord();
		result.addListElement(coord[0], coord[1]);
	} else if (propName == "-suppressErrors") {
		result = suppressErrors;
	} else {
		throw CommandException("No such property: ", propName);
	}
}

float OSDWidget::getRecursiveFadeValue() const
{
	return 1.0f; // fully opaque
}

void OSDWidget::invalidateRecursive()
{
	invalidateLocal();
	invalidateChildren();
}

void OSDWidget::invalidateChildren()
{
	for (auto& s : subWidgets) {
		s->invalidateRecursive();
	}
}

bool OSDWidget::needSuppressErrors() const
{
	if (suppressErrors) return true;
	if (const auto* p = getParent()) {
		return p->needSuppressErrors();
	}
	return false;
}

void OSDWidget::paintSDLRecursive(OutputSurface& output)
{
	paintSDL(output);

	optional<SDLScopedClip> scopedClip;
	if (clip) {
		vec2 clipPos, size;
		getBoundingBox(output, clipPos, size);
		scopedClip.emplace(output, clipPos, size);
	}

	for (auto& s : subWidgets) {
		s->paintSDLRecursive(output);
	}
}

void OSDWidget::paintGLRecursive (OutputSurface& output)
{
	(void)output;
#if COMPONENT_GL
	paintGL(output);

	optional<GLScopedClip> scopedClip;
	if (clip) {
		vec2 clipPos, size;
		getBoundingBox(output, clipPos, size);
		scopedClip.emplace(output, clipPos, size);
	}

	for (auto& s : subWidgets) {
		s->paintGLRecursive(output);
	}
#endif
}

int OSDWidget::getScaleFactor(const OutputSurface& output) const
{
	if (scaled) {
		return output.getLogicalSize()[0] / 320;;
	} else if (getParent()) {
		return getParent()->getScaleFactor(output);
	} else {
		return 1;
	}
}

vec2 OSDWidget::transformPos(const OutputSurface& output,
                             vec2 trPos, vec2 trRelPos) const
{
	vec2 out = trPos
	         + (float(getScaleFactor(output)) * getPos())
		 + (trRelPos * getSize(output));
	if (const auto* p = getParent()) {
		out = p->transformPos(output, out, getRelPos());
	}
	return out;
}

vec2 OSDWidget::transformReverse(const OutputSurface& output, vec2 trPos) const
{
	if (const auto* p = getParent()) {
		trPos = p->transformReverse(output, trPos);
		return trPos
		       - (getRelPos() * p->getSize(output))
		       - (getPos() * float(getScaleFactor(output)));
	} else {
		return trPos;
	}
}

vec2 OSDWidget::getMouseCoord() const
{
	if (SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE) {
		// Host cursor is not visible. Return dummy mouse coords for
		// the OSD cursor position.
		// The reason for doing this is that otherwise (e.g. when using
		// the mouse in an MSX program) it's possible to accidentally
		// click on the reversebar. This will also block the OSD mouse
		// in other Tcl scripts (e.g. vampier's nemesis script), but
		// almost always those scripts will also not be useful when the
		// host mouse cursor is not visible.
		//
		// We need to return coordinates that lay outside any
		// reasonable range. Initially we returned (NaN, NaN). But for
		// some reason that didn't work on dingoo: Dingoo uses
		// softfloat, in c++ NaN seems to behave as expected, but maybe
		// there's a problem on the tcl side? Anyway, when we return
		// +inf instead of NaN it does work.
		return vec2(std::numeric_limits<float>::infinity());
	}

	auto* output = getDisplay().getOutputSurface();
	if (!output) {
		throw CommandException(
			"Can't get mouse coordinates: no window visible");
	}

	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);

	vec2 out = transformReverse(*output, vec2(mouseX, mouseY));

	vec2 size = getSize(*output);
	if ((size[0] == 0.0f) || (size[1] == 0.0f)) {
		throw CommandException(
			"-can't get mouse coordinates: "
			"widget has zero width or height");
	}
	return out / size;
}

void OSDWidget::getBoundingBox(const OutputSurface& output,
                               vec2& bbPos, vec2& bbSize)
{
	vec2 topLeft     = transformPos(output, vec2(), vec2(0.0f));
	vec2 bottomRight = transformPos(output, vec2(), vec2(1.0f));
	bbPos  = topLeft;
	bbSize = bottomRight - topLeft;
}

} // namespace openmsx
