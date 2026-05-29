#include "SEditableText.h"

#include <chrono>

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SEditableText::construct(const Args& args) {
		m_text = args.initialText;
		m_cursorIndex = (int)m_text.length();
		m_hintText = args.hintText;
		m_textColor = args.textColor.value_or(GetTheme().textMain);
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().buttonPressed);
		m_focusedColor = args.focusedColor.value_or(GetTheme().buttonNormal);
		m_font = args.font;
		m_onTextChanged = args.onTextChanged;
		m_onTextCommitted = args.onTextCommitted;
	}

	void SEditableText::computeDesiredSize() {
		m_desiredSize = Vec2(150.0f, 30.0f);
	}

	void SEditableText::arrangeChildren(const Geometry & allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SEditableText::onDraw(DrawList & outDrawList, const Geometry & allocatedGeometry) const {
		if (!m_font) return;

		bool hasFocus = (SWidget::getFocusedWidget() == this);

		// -- Draw Background --
		addRectToDrawList(outDrawList, allocatedGeometry, hasFocus ? m_focusedColor : m_backgroundColor);

		// -- Push Clipping Rect For Text --
		outDrawList.pushClipRect(Rect(
			allocatedGeometry.position.x,
			allocatedGeometry.position.x + allocatedGeometry.size.x,
			allocatedGeometry.position.y,
			allocatedGeometry.position.y + allocatedGeometry.size.y
		));


		// -- Draw Text --
		std::string textToDraw = m_text.empty() && !hasFocus ? m_hintText : m_text;
		Color drawColor = m_text.empty() && !hasFocus ? GetTheme().textDim : m_textColor;

		float cursorX = allocatedGeometry.position.x + 5.0f;
		float baselineY = allocatedGeometry.position.y + 20.0f;

		float blinkingCursorPixelX = cursorX;

		for (size_t i = 0; i < textToDraw.length(); ++i) {
			char c = textToDraw[i];
			const Glyph& g = m_font->getGlyph(c);

			if (hasFocus && i == m_cursorIndex) {
				blinkingCursorPixelX = cursorX;
			}

			if (g.size.x > 0 && g.size.y > 0) {
				float x0 = cursorX + g.offset.x;
				float y0 = baselineY + g.offset.y;
				float x1 = x0 + g.size.x;
				float y1 = y0 + g.size.y;

				uint32_t startIndex = (uint32_t)outDrawList.vertices.size();
				outDrawList.vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, drawColor });
				outDrawList.vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, drawColor });
				outDrawList.vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, drawColor });
				outDrawList.vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, drawColor });

				outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 1); outDrawList.indices.push_back(startIndex + 2);
				outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 2); outDrawList.indices.push_back(startIndex + 3);

				if (outDrawList.commands.empty()) outDrawList.commands.push_back({ 0, 0, 0 });
				outDrawList.commands.back().indexCount += 6;
			}
			cursorX += g.advanceX;
		}

		if (hasFocus && m_cursorIndex == textToDraw.length()) {
			blinkingCursorPixelX = cursorX;
		}

		// -- Draw Blinking Cursor if Focused --
		auto now = std::chrono::steady_clock::now();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		if (hasFocus && (millis / 500) % 2 == 0) {
			Geometry cursorGeo = { {blinkingCursorPixelX, allocatedGeometry.position.y + 5.0f}, {2.0f, 20.0f} };
			addRectToDrawList(outDrawList, cursorGeo, m_textColor);
		}

		// -- Pop the Clipping Rect --
		outDrawList.popClipRect();
	}

	EventReply SEditableText::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (allocatedGeometry.contains(mousePos)) {
			// -- Force Cursor To Stay As I-Beam --
			Platform::setCursor(Platform::Cursor::TextInput);

			if (SWidget::getFocusedWidget() != this) {
				m_cursorIndex = (int)m_text.length();
				SWidget::setFocusedWidget(this);
			}

			// -- Calculate Cursor Position --
			if (m_font && !m_text.empty()) {
				float currentX = allocatedGeometry.position.x + 5.0f;
				int newIndex = 0;
				bool found = false;

				for (size_t i = 0; i < m_text.length(); ++i) {
					const Glyph& g = m_font->getGlyph(m_text[i]);

					// -- Snapping --
					if (mousePos.x < currentX + (g.advanceX * 0.5f)) {
						newIndex = (int)i;
						found = true;
						break;
					}
					currentX += g.advanceX;
				}

				if (!found) {
					newIndex = (int)m_text.length();
				}

				m_cursorIndex = newIndex;
			}
			else {
				m_cursorIndex = 0;
			}

			return EventReply::handled();
		}
		else if (SWidget::getFocusedWidget() == this) {
			if (m_onTextCommitted) m_onTextCommitted(m_text);
			SWidget::setFocusedWidget(nullptr);
		}

		return EventReply::unhandled();
	}

	EventReply SEditableText::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (allocatedGeometry.contains(mousePos)) {
			Platform::setCursor(Platform::Cursor::TextInput);
		}

		return EventReply::unhandled();
	}

	EventReply SEditableText::onChar(char c) {
		if (c == '\b') { // Backspace
			if (m_cursorIndex > 0) {
				m_text.erase(m_cursorIndex - 1, 1);
				m_cursorIndex--;
				if (m_onTextChanged) m_onTextChanged(m_text);
			}
		}
		else if (c >= 32 && c <= 126) {
			m_text.insert(m_cursorIndex, 1, c);
			m_cursorIndex++;
			if (m_onTextChanged) m_onTextChanged(m_text);
		}

		return EventReply::handled();
	}

	EventReply SEditableText::onKeyDown(Key key) {
		if (key == Key::Left) {
			if (m_cursorIndex > 0) m_cursorIndex--;
			return EventReply::handled();
		}
		else if (key == Key::Right) {
			if (m_cursorIndex < m_text.length()) m_cursorIndex++;
			return EventReply::handled();
		}
		else if (key == Key::Delete) {
			if (m_cursorIndex < m_text.length()) {
				m_text.erase(m_cursorIndex, 1);
				if (m_onTextChanged) m_onTextChanged(m_text);
			}
			return EventReply::handled();
		}
		else if (key == Key::Enter) {
			if (m_onTextCommitted) m_onTextCommitted(m_text);
			SWidget::setFocusedWidget(nullptr);
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	void SEditableText::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color });

		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 1); drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 2); drawList.indices.push_back(startIndex + 3);

		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 6;
	}

}
