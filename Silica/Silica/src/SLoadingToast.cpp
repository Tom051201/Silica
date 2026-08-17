#include "silicapch.h"
#include "SLoadingToast.h"

#include "Renderer.h"

namespace Silica {

	void SLoadingToast::construct(const Args& args) {
		m_text = args.text;
		m_font = args.font ? args.font : Silica::GetTheme().Font_Default;
		m_isVisible = args.isVisible;
	}

	void SLoadingToast::computeDesiredSize() {
		m_desiredSize = { 240.0f, 40.0f };
	}

	void SLoadingToast::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_isVisible && !m_isVisible()) return;

		// -- Draw Background --
		outDrawList.addRect(allocatedGeometry, GetTheme().Background_Popup);

		// -- Draw Borders --
		float t = GetTheme().Border_Thickness * m_renderScale; // Scale border by render scale
		Color c = GetTheme().Border_Selected;
		float x = allocatedGeometry.position.x;
		float y = allocatedGeometry.position.y;
		float w = allocatedGeometry.size.x;
		float h = allocatedGeometry.size.y;

		outDrawList.addRect({ {x, y}, {w, t} }, c); // Top
		outDrawList.addRect({ {x, y + h - t}, {w, t} }, c); // Bottom
		outDrawList.addRect({ {x, y + t}, {t, h - (t * 2)} }, c); // Left
		outDrawList.addRect({ {x + w - t, y + t}, {t, h - (t * 2)} }, c); // Right

		// -- Draw Text --
		if (m_font) {
			const Glyph& g = m_font->getGlyph('C');
			float textY = y + (h * 0.5f) - (g.size.y * m_renderScale * 0.5f) - (g.offset.y * m_renderScale);

			Vec2 textPos = { x + (42.0f * m_renderScale), std::round(textY) };
			outDrawList.addText(m_font, m_text, textPos, GetTheme().Text_Main, m_renderScale);
		}

		// -- Draw Rotating Spinner --
		Vec2 center = { x + (20.0f * m_renderScale), y + (20.0f * m_renderScale) };
		float radius = 7.0f * m_renderScale;
		float thickness = 2.5f * m_renderScale;
		Color spinnerCol = GetTheme().Accent_Primary;

		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float>(currentTime - startTime).count();

		int segments = 24;
		float startAngle = time * 6.0f;
		float endAngle = startAngle + (3.14159f * 1.5f);

		uint32_t startIndex = (uint32_t)outDrawList.vertices.size();
		for (int i = 0; i <= segments; i++) {
			float tStep = (float)i / segments;
			float angle = startAngle + (endAngle - startAngle) * tStep;

			float cosA = std::cos(angle);
			float sinA = std::sin(angle);

			Vec2 inner = { center.x + cosA * (radius - thickness), center.y + sinA * (radius - thickness) };
			Vec2 outer = { center.x + cosA * radius, center.y + sinA * radius };

			outDrawList.vertices.push_back({ inner, {0, 0}, spinnerCol });
			outDrawList.vertices.push_back({ outer, {0, 0}, spinnerCol });
		}

		for (int i = 0; i < segments; i++) {
			uint32_t i0 = startIndex + i * 2;
			uint32_t i1 = i0 + 1;
			uint32_t i2 = i0 + 2;
			uint32_t i3 = i0 + 3;

			outDrawList.indices.push_back(i0); outDrawList.indices.push_back(i1); outDrawList.indices.push_back(i2);
			outDrawList.indices.push_back(i1); outDrawList.indices.push_back(i3); outDrawList.indices.push_back(i2);
		}

		if (outDrawList.commands.empty()) outDrawList.commands.push_back({ 0, 0, 0 });
		outDrawList.commands.back().indexCount += segments * 6;
	}

}
