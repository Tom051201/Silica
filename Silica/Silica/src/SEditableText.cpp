#include "SEditableText.h"

#include <chrono>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SEditableText::construct(const Args& args) {
		m_text = args.initialText;
		m_cursorIndex = (int)m_text.length();
		m_hintText = args.hintText;
		m_textColor = args.textColor.value_or(GetTheme().Text_Main);
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().Background_Input);
		m_focusedColor = args.focusedColor.value_or(GetTheme().Element_Hover);
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_onTextChanged = args.onTextChanged;
		m_onTextCommitted = args.onTextCommitted;
	}

	void SEditableText::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f, 30.0f);
	}

	void SEditableText::arrangeChildren(const Geometry & allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SEditableText::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (!m_font) return;

		bool hasFocus = (SWidget::getFocusedWidget() == this);

		// -- Draw Background --
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
		if (hasFocus && !m_text.empty()) {
			for (int i = 0; i < m_cursorIndex && i < textToDraw.length(); ++i) {
				cursorPixelOffset += m_font->getGlyph(textToDraw[i]).advanceX;
			}
		}

		if (hasFocus) {
			float maxVisibleWidth = allocatedGeometry.size.x - 10.0f;

			if (cursorPixelOffset - m_scrollOffset < 0.0f) {
				m_scrollOffset = cursorPixelOffset;
				m_scrollOffset = cursorPixelOffset;
			}
			else if (cursorPixelOffset - m_scrollOffset > maxVisibleWidth) {
				m_scrollOffset = cursorPixelOffset - maxVisibleWidth;
			}
		}
		else {
			m_scrollOffset = 0.0f;
		}

		float startX = allocatedGeometry.position.x + 5.0f - m_scrollOffset;
		float baselineY = allocatedGeometry.position.y + 20.0f;

		// -- Draw Selection Highlight --
		if (hasFocus && hasSelection() && !m_text.empty()) {
			int startIdx = std::min(m_cursorIndex, m_selectionAnchor);
			int endIdx = std::max(m_cursorIndex, m_selectionAnchor);

			float pX1 = startX;
			float pX2 = startX;

			for (int i = 0; i < startIdx; ++i) pX1 += m_font->getGlyph(textToDraw[i]).advanceX;
			for (int i = 0; i < endIdx; ++i) pX2 += m_font->getGlyph(textToDraw[i]).advanceX;

			Color highlightColor = GetTheme().Accent_Primary;
			highlightColor.setAlpha(120);

			Geometry selGeo = { {pX1, allocatedGeometry.position.y + 4.0f}, {pX2 - pX1, 22.0f} };
			outDrawList.addRect(selGeo, highlightColor);
		}

		// -- Draw Text --
		outDrawList.addText(m_font, textToDraw, { startX, baselineY }, drawColor);

		// -- Draw Blinking Cursor --
		float blinkingCursorPixelX = startX;
		if (hasFocus) {
			for (size_t i = 0; i < m_cursorIndex && i < textToDraw.length(); ++i) {
				blinkingCursorPixelX += m_font->getGlyph(textToDraw[i]).advanceX;
			}
		}

		auto now = std::chrono::steady_clock::now();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		bool forceCursor = m_isDragging || m_isShiftDown || m_isCtrlDown;
		if (hasFocus && (forceCursor || (millis / 500) % 2 == 0)) {
			float blinkingCursorPixelX = startX + cursorPixelOffset;
			Geometry cursorGeo = { {blinkingCursorPixelX, allocatedGeometry.position.y + 5.0f}, {2.0f, 20.0f} };
			outDrawList.addRect(cursorGeo, m_textColor);
		}

		outDrawList.popClipRect();
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
		if (c == '\b') { // Backspace
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

		if (key == Key::A && m_isCtrlDown) {
			m_selectionAnchor = 0;
			m_cursorIndex = (int)m_text.length();
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

		float currentX = geo.position.x + 5.0f - m_scrollOffset;

		for (size_t i = 0; i < m_text.length(); ++i) {
			const Glyph& g = m_font->getGlyph(m_text[i]);
			if (pos.x < currentX + (g.advanceX * 0.5f)) {
				return (int)i;
			}
			currentX += g.advanceX;
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
