#include "STextBlock.h"

#include "Renderer.h"

namespace Silica {

	void STextBlock::construct(const Args& args) {
		m_text = args.text;
		m_color = args.color;
		m_font = args.font;
	}

	void STextBlock::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (!m_font || m_text.empty()) return;

		m_desiredSize.y = 20.0f;

		for (char c : m_text) {
			const Glyph& g = m_font->getGlyph(c);
			m_desiredSize.x += g.advanceX;
		}
	}

	void STextBlock::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void STextBlock::onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const {
		if (!m_font || m_text.empty() || m_color.a() == 0) return;

		float cursorX = allotedGeometry.position.x;
		float baselineY = allotedGeometry.position.y + 16.0f;

		for (char c : m_text) {
			const Glyph& g = m_font->getGlyph(c);

			if (g.size.x > 0 && g.size.y > 0) {

				float x0 = cursorX + g.offset.x;
				float y0 = baselineY + g.offset.y;
				float x1 = x0 + g.size.x;
				float y1 = y0 + g.size.y;

				uint32_t startIndex = (uint32_t)outDrawList.vertices.size();

				outDrawList.vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, m_color }); // TL
				outDrawList.vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, m_color }); // TR
				outDrawList.vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, m_color }); // BR
				outDrawList.vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, m_color }); // BL

				outDrawList.indices.push_back(startIndex + 0);
				outDrawList.indices.push_back(startIndex + 1);
				outDrawList.indices.push_back(startIndex + 2);
				outDrawList.indices.push_back(startIndex + 0);
				outDrawList.indices.push_back(startIndex + 2);
				outDrawList.indices.push_back(startIndex + 3);

				if (outDrawList.commands.empty()) {
					outDrawList.commands.push_back({ 0, 0, 0 });
				}
				outDrawList.commands.back().indexCount += 6;
			}

			cursorX += g.advanceX;
		}
	}

}
