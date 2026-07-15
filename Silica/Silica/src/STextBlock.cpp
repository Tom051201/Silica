#include "STextBlock.h"

#include "Renderer.h"
#include "Theme.h"

#include <cmath>

namespace Silica {

	void STextBlock::construct(const Args& args) {
		m_text = args.text;
		m_color = args.color.value_or(GetTheme().Text_Main);
		m_font = args.font ? args.font : GetTheme().Font_Default;
	}

	void STextBlock::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (!m_font || m_text.empty()) return;

		float lineHeight = 20.0f;
		float currentWidth = 0.0f;
		float maxWidth = 0.0f;

		m_desiredSize.y = lineHeight;

		for (char c : m_text) {
			if (c == '\n') {
				maxWidth = std::max(maxWidth, currentWidth);
				currentWidth = 0.0f;
				m_desiredSize.y += lineHeight;
			}
			else {
				const Glyph& g = m_font->getGlyph(c);
				currentWidth += g.advanceX;
			}
		}

		maxWidth = std::max(maxWidth, currentWidth);
		m_desiredSize.x = maxWidth;
	}

	void STextBlock::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void STextBlock::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (!m_font || m_text.empty() || m_color.a() == 0) return;

		Vec2 textPos = { allocatedGeometry.position.x, allocatedGeometry.position.y + 16.0f };

		outDrawList.addText(m_font, m_text, textPos, m_color);
	}

	void STextBlock::setText(const std::string& text) {
		m_text = text;
	}

}
