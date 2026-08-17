#include "SEditableText.h"

#include <chrono>
#include <algorithm>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SEditableText::construct(const Args& args) {
		m_text = args.initialText;
		m_cursorIndex = (int)m_text.length();
		m_selectionAnchor = m_cursorIndex;
		m_hintText = args.hintText;
		m_textColor = args.textColor.value_or(GetTheme().Text_Main);
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().Background_Input);
		m_focusedColor = args.focusedColor.value_or(GetTheme().Element_Hover);
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_charFilter = args.charFilter;
		m_onTextChanged = args.onTextChanged;
		m_onTextCommitted = args.onTextCommitted;
	}

	void SEditableText::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f, 30.0f);
	}

	void SEditableText::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SEditableText::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (!m_font) return;
		bool hasFocus = (SWidget::getFocusedWidget() == this);

		outDrawList.addRect(allocatedGeometry, hasFocus ? m_focusedColor : m_backgroundColor);

		outDrawList.pushClipRect(Rect(
			allocatedGeometry.position.x,
			allocatedGeometry.position.x + allocatedGeometry.size.x,
			allocatedGeometry.position.y,
			allocatedGeometry.position.y + allocatedGeometry.size.y
		));

		std::string textToDraw = m_text.empty() && !hasFocus ? m_hintText : m_text;
		Color drawColor = m_text.empty() && !hasFocus ? GetTheme().Text_Dim : m_textColor;

		float cursorPixelOffset = 0.0f;
		for (int i = 0; i < m_cursorIndex && i < (int)textToDraw.length(); ++i) {
			cursorPixelOffset += m_font->getGlyph(textToDraw[i]).advanceX * m_renderScale;
		}

		if (hasFocus) {
			float maxVisibleWidth = std::max(1.0f, allocatedGeometry.size.x - (10.0f * m_renderScale));

			if (cursorPixelOffset - m_scrollOffset < 0.0f) {
				m_scrollOffset = cursorPixelOffset;
			}
			else if (cursorPixelOffset - m_scrollOffset > maxVisibleWidth) {
				m_scrollOffset = cursorPixelOffset - maxVisibleWidth;
			}
		}
		else {
			m_scrollOffset = 0.0f;
		}

		float startX = allocatedGeometry.position.x + (5.0f * m_renderScale) - m_scrollOffset;
		float baselineY = allocatedGeometry.position.y + (20.0f * m_renderScale);

		// -- Pro Selection Drawing (Highlight + White Text) --
		if (hasFocus && hasSelection() && !m_text.empty()) {
			int startIdx = std::min(m_cursorIndex, m_selectionAnchor);
			int endIdx = std::max(m_cursorIndex, m_selectionAnchor);

			float pX1 = startX;
			float pX2 = startX;

			for (int i = 0; i < startIdx; ++i) pX1 += m_font->getGlyph(textToDraw[i]).advanceX * m_renderScale;
			for (int i = 0; i < endIdx; ++i) pX2 += m_font->getGlyph(textToDraw[i]).advanceX * m_renderScale;

			// Highly visible solid blue highlight
			Color highlightColor = Color(0, 120, 215, 255);

			Geometry selGeo = {
				{pX1, allocatedGeometry.position.y + (4.0f * m_renderScale)},
				{pX2 - pX1, 22.0f * m_renderScale}
			};
			outDrawList.addRect(selGeo, highlightColor);

			// Split string into 3 parts so the selected text pops in white
			std::string beforeText = textToDraw.substr(0, startIdx);
			std::string selectedText = textToDraw.substr(startIdx, endIdx - startIdx);
			std::string afterText = textToDraw.substr(endIdx);

			if (!beforeText.empty()) outDrawList.addText(m_font, beforeText, { startX, baselineY }, drawColor, m_renderScale, 20.0f * m_renderScale);
			if (!selectedText.empty()) outDrawList.addText(m_font, selectedText, { pX1, baselineY }, Color::white(), m_renderScale, 20.0f * m_renderScale);
			if (!afterText.empty()) outDrawList.addText(m_font, afterText, { pX2, baselineY }, drawColor, m_renderScale, 20.0f * m_renderScale);
		}
		else {
			// Normal unselected text
			outDrawList.addText(m_font, textToDraw, { startX, baselineY }, drawColor, m_renderScale, 20.0f * m_renderScale);
		}

		// -- Draw Blinking Cursor --
		auto now = std::chrono::steady_clock::now();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		bool forceCursor = m_isDragging || m_isShiftDown || m_isCtrlDown;
		if (hasFocus && (forceCursor || (millis / 500) % 2 == 0)) {
			float blinkingCursorPixelX = startX + cursorPixelOffset;
			Geometry cursorGeo = {
				{blinkingCursorPixelX, allocatedGeometry.position.y + (5.0f * m_renderScale)},
				{std::max(1.0f, 2.0f * m_renderScale), 20.0f * m_renderScale}
			};
			outDrawList.addRect(cursorGeo, m_textColor);
		}

		outDrawList.popClipRect();
	}

	void SEditableText::setText(const std::string& text, bool moveToEnd) {
		m_text = text;

		if (moveToEnd) {
			m_cursorIndex = (int)m_text.length();
			m_selectionAnchor = (int)m_text.length();
		}
		else {
			m_cursorIndex = std::clamp(m_cursorIndex, 0, (int)m_text.length());
			m_selectionAnchor = std::clamp(m_selectionAnchor, 0, (int)m_text.length());
		}

		m_scrollOffset = 0.0f;
	}

	EventReply SEditableText::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (allocatedGeometry.contains(mousePos) || m_isDragging) {
			Platform::setCursor(Platform::Cursor::TextInput);
		}

		if (m_isDragging) {
			m_cursorIndex = getIndexFromMousePos(allocatedGeometry, mousePos);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SEditableText::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (allocatedGeometry.contains(mousePos)) {
			// -- Force Cursor To Stay As I-Beam --
			Platform::setCursor(Platform::Cursor::TextInput);

			if (SWidget::getFocusedWidget() != this) {
				SWidget::setFocusedWidget(this);
			}

			m_cursorIndex = getIndexFromMousePos(allocatedGeometry, mousePos);

			if (!m_isShiftDown) {
				m_selectionAnchor = m_cursorIndex;
			}

			m_isDragging = true;
			SWidget::setCapturedWidget(this);

			return EventReply::handled();
		}
		else if (SWidget::getFocusedWidget() == this) {
			if (m_onTextCommitted) m_onTextCommitted(m_text);
			SWidget::setFocusedWidget(nullptr);
			m_selectionAnchor = m_cursorIndex;
		}

		return EventReply::unhandled();
	}

	EventReply SEditableText::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button == MouseButton::Left && m_isDragging) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SEditableText::onChar(char c) {
		if (c == '\b') {
			if (hasSelection()) {
				deleteSelection();
			}
			else if (m_cursorIndex > 0) {
				m_text.erase(m_cursorIndex - 1, 1);
				m_cursorIndex--;
				m_selectionAnchor = m_cursorIndex;
				if (m_onTextChanged) m_onTextChanged(m_text);
			}
		}
		else if (c >= 32 && c <= 126) {
			if (m_charFilter && !m_charFilter(c)) {
				return EventReply::unhandled();
			}

			if (hasSelection()) deleteSelection();

			m_text.insert(m_cursorIndex, 1, c);
			m_cursorIndex++;
			m_selectionAnchor = m_cursorIndex;
			if (m_onTextChanged) m_onTextChanged(m_text);
		}

		return EventReply::handled();
	}

	EventReply SEditableText::onKeyDown(Key key) {
		if (key == Key::LeftShift || key == Key::RightShift) {
			m_isShiftDown = true;
			return EventReply::handled();
		}

		if (key == Key::LeftControl || key == Key::RightControl) {
			m_isCtrlDown = true;
			return EventReply::handled();
		}

		// -- Select All --
		if (key == Key::A && m_isCtrlDown) {
			m_selectionAnchor = 0;
			m_cursorIndex = (int)m_text.length();
			return EventReply::handled();
		}

		// -- Copy - Ctrl + C --
		if (key == Key::C && m_isCtrlDown) {
			if (hasSelection()) {
				int start = std::min(m_cursorIndex, m_selectionAnchor);
				int end = std::max(m_cursorIndex, m_selectionAnchor);
				Platform::setClipboardText(m_text.substr(start, end - start));
			}
			return EventReply::handled();
		}

		// -- Cut - Ctrl + X --
		if (key == Key::X && m_isCtrlDown) {
			if (hasSelection()) {
				int start = std::min(m_cursorIndex, m_selectionAnchor);
				int end = std::max(m_cursorIndex, m_selectionAnchor);
				Platform::setClipboardText(m_text.substr(start, end - start));
				deleteSelection();
			}
			return EventReply::handled();
		}

		// -- Paste - Ctrl + V --
		if (key == Key::V && m_isCtrlDown) {
			std::string pasteText = Platform::getClipboardText();
			if (!pasteText.empty()) {
				// -- Delete Selection --
				if (hasSelection()) deleteSelection();

				// -- Filter Incoming Text --
				std::string filteredPaste = "";
				for (char c : pasteText) {
					if (c >= 32 && c <= 126) {
						if (!m_charFilter || m_charFilter(c)) {
							filteredPaste += c;
						}
					}
				}

				// -- Insert String --
				if (!filteredPaste.empty()) {
					m_text.insert(m_cursorIndex, filteredPaste);
					m_cursorIndex += (int)filteredPaste.length();
					m_selectionAnchor = m_cursorIndex;
					if (m_onTextChanged) m_onTextChanged(m_text);
				}
			}
			return EventReply::handled();
		}

		if (key == Key::Left) {
			if (m_isShiftDown) {
				if (m_cursorIndex > 0) m_cursorIndex--;
			}
			else {
				if (hasSelection()) m_cursorIndex = std::min(m_cursorIndex, m_selectionAnchor);
				else if (m_cursorIndex > 0) m_cursorIndex--;
				m_selectionAnchor = m_cursorIndex;
			}
			return EventReply::handled();
		}
		else if (key == Key::Right) {
			if (m_isShiftDown) {
				if (m_cursorIndex < m_text.length()) m_cursorIndex++;
			}
			else {
				if (hasSelection()) m_cursorIndex = std::max(m_cursorIndex, m_selectionAnchor);
				else if (m_cursorIndex < m_text.length()) m_cursorIndex++;
				m_selectionAnchor = m_cursorIndex;
			}
			return EventReply::handled();
		}
		else if (key == Key::Delete) {
			if (hasSelection()) {
				deleteSelection();
			}
			else if (m_cursorIndex < m_text.length()) {
				m_text.erase(m_cursorIndex, 1);
				if (m_onTextChanged) m_onTextChanged(m_text);
			}
			return EventReply::handled();
		}
		else if (key == Key::Enter) {
			if (m_onTextCommitted) m_onTextCommitted(m_text);
			SWidget::setFocusedWidget(nullptr);
			m_selectionAnchor = m_cursorIndex;
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SEditableText::onKeyUp(Key key) {
		if (key == Key::LeftShift || key == Key::RightShift) {
			m_isShiftDown = false;
			return EventReply::handled();
		}
		if (key == Key::LeftControl || key == Key::RightControl) {
			m_isCtrlDown = false;
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	int SEditableText::getIndexFromMousePos(const Geometry& geo, const Vec2& pos) const {
		if (!m_font || m_text.empty()) return 0;

		float currentX = geo.position.x + (5.0f * m_renderScale) - m_scrollOffset;

		for (size_t i = 0; i < m_text.length(); ++i) {
			const Glyph& g = m_font->getGlyph(m_text[i]);
			if (pos.x < currentX + (g.advanceX * m_renderScale * 0.5f)) {
				return (int)i;
			}
			currentX += g.advanceX * m_renderScale;
		}
		return (int)m_text.length();
	}

	bool SEditableText::hasSelection() const {
		return m_selectionAnchor != m_cursorIndex;
	}

	void SEditableText::deleteSelection() {
		if (!hasSelection()) return;

		int start = std::min(m_cursorIndex, m_selectionAnchor);
		int end = std::max(m_cursorIndex, m_selectionAnchor);

		m_text.erase(start, end - start);
		m_cursorIndex = start;
		m_selectionAnchor = start;

		if (m_onTextChanged) m_onTextChanged(m_text);
	}

}
