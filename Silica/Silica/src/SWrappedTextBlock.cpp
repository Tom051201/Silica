#include "silicapch.h"
#include "SWrappedTextBlock.h"

#include "Renderer.h"

namespace Silica {

	void SWrappedTextBlock::construct(const Args& args) {
		m_text = args.text;
		m_wrapWidth = args.wrapWidth;
		m_maxLines = args.maxLines;
		m_color = args.color.value_or(GetTheme().Text_Main);
		m_font = args.font ? args.font : GetTheme().Font_Default;
		updateWrappedText();
	}

	void SWrappedTextBlock::updateWrappedText() {
		if (!m_font || m_text.empty() || m_wrapWidth <= 0.0f) {
			m_wrappedText = m_text;
			return;
		}

		m_wrappedText.clear();
		float currentLineWidth = 0.0f;
		int currentLine = 1;

		size_t start = 0;
		size_t end = 0;
		while (start < m_text.length()) {
			end = m_text.find_first_of(" \n", start);
			if (end == std::string::npos) end = m_text.length();

			std::string word = m_text.substr(start, end - start);
			char delimiter = (end < m_text.length()) ? m_text[end] : '\0';

			float wordWidth = 0.0f;
			for (char c : word) wordWidth += m_font->getGlyph(c).advanceX;
			float spaceWidth = m_font->getGlyph(' ').advanceX;

			if (currentLineWidth + wordWidth > m_wrapWidth && currentLineWidth > 0.0f) {
				if (m_maxLines > 0 && currentLine >= m_maxLines) {
					while (!m_wrappedText.empty() && (m_wrappedText.back() == ' ' || m_wrappedText.back() == '\n')) {
						m_wrappedText.pop_back();
					}
					m_wrappedText += "...";
					return;
				}
				m_wrappedText += "\n";
				currentLineWidth = 0.0f;
				currentLine++;
			}

			m_wrappedText += word;
			currentLineWidth += wordWidth;

			if (delimiter == ' ') {
				m_wrappedText += " ";
				currentLineWidth += spaceWidth;
			}
			else if (delimiter == '\n') {
				if (m_maxLines > 0 && currentLine >= m_maxLines) {
					while (!m_wrappedText.empty() && (m_wrappedText.back() == ' ' || m_wrappedText.back() == '\n')) {
						m_wrappedText.pop_back();
					}
					m_wrappedText += "...";
					return;
				}
				m_wrappedText += "\n";
				currentLineWidth = 0.0f;
				currentLine++;
			}

			start = end + 1;
		}
	}

	void SWrappedTextBlock::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (!m_font || m_wrappedText.empty()) return;

		float lineHeight = 20.0f;
		float currentWidth = 0.0f;
		float maxWidth = 0.0f;

		m_desiredSize.y = lineHeight;

		for (char c : m_wrappedText) {
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

	void SWrappedTextBlock::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SWrappedTextBlock::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (!m_font || m_wrappedText.empty() || m_color.a() == 0) return;

		Vec2 textPos = { allocatedGeometry.position.x, allocatedGeometry.position.y + (16.0f * m_renderScale) };
		outDrawList.addText(m_font, m_wrappedText, textPos, m_color, m_renderScale, 20.0f * m_renderScale);
	}

	void SWrappedTextBlock::setText(const std::string& text) {
		m_text = text;
		updateWrappedText();
	}

}
