#include "SColorPicker.h"

#include <algorithm>
#include <cmath>

namespace Silica {

	void SColorPicker::construct(const Args& args) {
		rgbToHsv(args.initialColor, m_h, m_s, m_v, m_a);
		m_onColorChanged = args.onColorChanged;
	}

	void SColorPicker::computeDesiredSize() {
		// 150px SV Box + 5px gap + 20px Hue + 5px gap + 20px Alpha = 200px Total Height
		m_desiredSize = Vec2(200.0f, 200.0f);
	}

	void SColorPicker::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SColorPicker::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		Rect svRect = getSVBoxRect(allocatedGeometry);
		Rect hueRect = getHueBarRect(allocatedGeometry);
		Rect alphaRect = getAlphaBarRect(allocatedGeometry);

		// -- Draw SV Box (Saturation / Value) --
		Color pureHue = hsvToRgb(m_h, 1.0f, 1.0f, 1.0f);
		Geometry svGeo = { {svRect.left, svRect.top}, {svRect.getWidth(), svRect.getHeight()} };
		addGradientRect(outDrawList, svGeo, Color::white(), pureHue, Color::black(), Color::black());

		// -- Draw Picker Reticle Inside SV Box --
		Vec2 reticlePos(
			svRect.left + (m_s * svRect.getWidth()),
			svRect.top + ((1.0f - m_v) * svRect.getHeight())
		);
		addRectToDrawList(outDrawList, { {reticlePos.x - 3.0f, reticlePos.y - 3.0f}, {6.0f, 6.0f} }, Color::black());
		addRectToDrawList(outDrawList, { {reticlePos.x - 2.0f, reticlePos.y - 2.0f}, {4.0f, 4.0f} }, Color::white());


		// -- Draw Hue Bar --
		int segments = 6;
		float segWidth = hueRect.getWidth() / segments;
		Color hueColors[7] = {
			Color(255, 0, 0), Color(255, 255, 0), Color(0, 255, 0),
			Color(0, 255, 255), Color(0, 0, 255), Color(255, 0, 255), Color(255, 0, 0)
		};

		for (int i = 0; i < segments; ++i) {
			Geometry segGeo = { {hueRect.left + i * segWidth, hueRect.top}, {segWidth, hueRect.getHeight()} };
			addGradientRect(outDrawList, segGeo, hueColors[i], hueColors[i + 1], hueColors[i + 1], hueColors[i]);
		}

		// -- Draw Hue Thumb --
		float hueX = hueRect.left + (m_h * hueRect.getWidth());
		addRectToDrawList(outDrawList, { {hueX - 2.0f, hueRect.top - 2.0f}, {4.0f, hueRect.getHeight() + 4.0f} }, Color::white());


		// -- Draw Alpha Bar --
		Color alphaZero = hsvToRgb(m_h, m_s, m_v, 0.0f);
		Color alphaFull = hsvToRgb(m_h, m_s, m_v, 1.0f);
		Geometry alphaGeo = { {alphaRect.left, alphaRect.top}, {alphaRect.getWidth(), alphaRect.getHeight()} };
		addGradientRect(outDrawList, alphaGeo, alphaZero, alphaFull, alphaFull, alphaZero);

		// -- Draw Alpha Thumb --
		float alphaX = alphaRect.left + (m_a * alphaRect.getWidth());
		addRectToDrawList(outDrawList, { {alphaX - 2.0f, alphaRect.top - 2.0f}, {4.0f, alphaRect.getHeight() + 4.0f} }, Color::white());
	}

	void SColorPicker::updateFromMouse(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		bool changed = false;

		if (m_dragState == DragState::SVBox) {
			Rect r = getSVBoxRect(allocatedGeometry);
			m_s = std::clamp((mousePos.x - r.left) / r.getWidth(), 0.0f, 1.0f);
			m_v = 1.0f - std::clamp((mousePos.y - r.top) / r.getHeight(), 0.0f, 1.0f);
			changed = true;
		}
		else if (m_dragState == DragState::HueBar) {
			Rect r = getHueBarRect(allocatedGeometry);
			m_h = std::clamp((mousePos.x - r.left) / r.getWidth(), 0.0f, 1.0f);
			changed = true;
		}
		else if (m_dragState == DragState::AlphaBar) {
			Rect r = getAlphaBarRect(allocatedGeometry);
			m_a = std::clamp((mousePos.x - r.left) / r.getWidth(), 0.0f, 1.0f);
			changed = true;
		}

		if (changed && m_onColorChanged) {
			m_onColorChanged(hsvToRgb(m_h, m_s, m_v, m_a));
		}
	}

	EventReply SColorPicker::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_dragState != DragState::None) {
			updateFromMouse(allocatedGeometry, mousePos);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SColorPicker::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (getSVBoxRect(allocatedGeometry).contains(mousePos)) m_dragState = DragState::SVBox;
		else if (getHueBarRect(allocatedGeometry).contains(mousePos)) m_dragState = DragState::HueBar;
		else if (getAlphaBarRect(allocatedGeometry).contains(mousePos)) m_dragState = DragState::AlphaBar;
		else return EventReply::unhandled();

		SWidget::setCapturedWidget(this);
		updateFromMouse(allocatedGeometry, mousePos);

		return EventReply::handled();
	}

	EventReply SColorPicker::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (m_dragState != DragState::None) {
			m_dragState = DragState::None;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}



	// ----- Layout Helpers -----
	Rect SColorPicker::getSVBoxRect(const Geometry& geo) const {
		return Rect(geo.position.x, geo.position.x + geo.size.x, geo.position.y, geo.position.y + geo.size.y - 50.0f);
	}

	Rect SColorPicker::getHueBarRect(const Geometry& geo) const {
		return Rect(geo.position.x, geo.position.x + geo.size.x, geo.position.y + geo.size.y - 45.0f, geo.position.y + geo.size.y - 25.0f);
	}

	Rect SColorPicker::getAlphaBarRect(const Geometry& geo) const {
		return Rect(geo.position.x, geo.position.x + geo.size.x, geo.position.y + geo.size.y - 20.0f, geo.position.y + geo.size.y);
	}



	// ----- Gradient Hardware Renderer -----
	void SColorPicker::addGradientRect(DrawList& drawList, const Geometry& geo, Color tl, Color tr, Color br, Color bl) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, tl });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, tr });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, br });
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, bl });

		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 1); drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 2); drawList.indices.push_back(startIndex + 3);
		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 6;
	}

	void SColorPicker::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
		addGradientRect(drawList, geo, color, color, color, color);
	}



	// ----- Color Math -----
	Color SColorPicker::hsvToRgb(float h, float s, float v, float a) const {
		if (s <= 0.0f) {
			uint8_t val = (uint8_t)(v * 255.0f);
			return Color(val, val, val, (uint8_t)(a * 255.0f));
		}
		float hh = h * 6.0f;
		if (hh >= 6.0f) hh = 0.0f;
		int i = (int)hh;
		float ff = hh - i;
		float p = v * (1.0f - s);
		float q = v * (1.0f - (s * ff));
		float t = v * (1.0f - (s * (1.0f - ff)));
		float r = 0, g = 0, b = 0;

		switch (i) {
			case 0: r = v; g = t; b = p; break;
			case 1: r = q; g = v; b = p; break;
			case 2: r = p; g = v; b = t; break;
			case 3: r = p; g = q; b = v; break;
			case 4: r = t; g = p; b = v; break;
			case 5: r = v; g = p; b = q; break;
		}
		return Color((uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f), (uint8_t)(a * 255.0f));
	}

	void SColorPicker::rgbToHsv(Color c, float& h, float& s, float& v, float& a) const {
		float r = c.r() / 255.0f, g = c.g() / 255.0f, b = c.b() / 255.0f;
		a = c.a() / 255.0f;
		float cmax = std::max({ r, g, b }), cmin = std::min({ r, g, b });
		float diff = cmax - cmin;
		v = cmax;
		if (cmax == 0.0f) { s = 0.0f; h = 0.0f; return; }
		s = diff / cmax;
		if (diff == 0.0f) h = 0.0f;
		else if (cmax == r) h = fmod(((g - b) / diff), 6.0f);
		else if (cmax == g) h = ((b - r) / diff) + 2.0f;
		else if (cmax == b) h = ((r - g) / diff) + 4.0f;
		h /= 6.0f;
		if (h < 0.0f) h += 1.0f;
	}

}
