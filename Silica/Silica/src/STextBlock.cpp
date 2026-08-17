#include "STextBlock.h"

#include "Renderer.h"
#include "Theme.h"

#include <cmath>

namespace Silica {

	void STextBlock::construct(const Args& args) {
		m_text = args.text;
		m_color = args.color.value_or(GetTheme().Text_Main);
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_truncateWidth = args.truncateWidth;
		updateDisplayText();
	}

	void STextBlock::updateDisplayText() {
		if (m_truncateWidth <= 0.0f || !m_font) {
			m_displayText = m_text;
			return;
		}

		float currentWidth = 0.0f;
		float dotWidth = m_font->getGlyph('.').advanceX * 3.0f;
		m_displayText.clear();

		for (size_t i = 0; i < m_text.size(); ++i) {
			float charWidth = m_font->getGlyph(m_text[i]).advanceX;
			if (currentWidth + charWidth > m_truncateWidth - dotWidth && i < m_text.size() - 1) {
				m_displayText += "...";
				break;
			}
			m_displayText += m_text[i];
			currentWidth += charWidth;
		}
	}

	void STextBlock::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (!m_font || m_displayText.empty()) return;

		float lineHeight = 20.0f;
		float currentWidth = 0.0f;
		float maxWidth = 0.0f;

		m_desiredSize.y = lineHeight;

		for (char c : m_displayText) {
			if (c == '\n') {
				maxWidth = std::max(maxWidth, currentWidth);
				currentWidth = 0.0f;
				m_desiredSize.y += lineHeight;
			}
			else {
				currentWidth += m_font->getGlyph(c).advanceX;
			}
		}

		maxWidth = std::max(maxWidth, currentWidth);
		m_desiredSize.x = maxWidth;
	}

	void STextBlock::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void STextBlock::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (!m_font || m_displayText.empty() || m_color.a() == 0) return;

		Vec2 textPos = { allocatedGeometry.position.x, allocatedGeometry.position.y + (16.0f * m_renderScale) };
		outDrawList.addText(m_font, m_displayText, textPos, m_color, m_renderScale, 20.0f * m_renderScale);
	}

	void STextBlock::setText(const std::string& text) {
		m_text = text;
		updateDisplayText();
	}

}
