#include "SEditableText.h"

#include <chrono>

#include "Renderer.h"

namespace Silica {

	void SEditableText::construct(const Args& args) {
		m_hintText = args.hintText;
		m_textColor = args.textColor;
		m_backgroundColor = args.backgroundColor;
		m_focusedColor = args.focusedColor;
		m_font = args.font;
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
		Rect clipRect(
			allocatedGeometry.position.x,
			allocatedGeometry.position.x + allocatedGeometry.size.x,
			allocatedGeometry.position.y,
			allocatedGeometry.position.y + allocatedGeometry.size.y
		);
		outDrawList.pushClipRect(clipRect);


		// -- Draw Text --
		std::string textToDraw = m_text.empty() && !hasFocus ? m_hintText : m_text;
		Color drawColor = m_text.empty() && !hasFocus ? Color(150, 150, 150) : m_textColor;

		float cursorX = allocatedGeometry.position.x + 5.0f;
		float baselineY = allocatedGeometry.position.y + 20.0f;

		for (char c : textToDraw) {
			const Glyph& g = m_font->getGlyph(c);
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

		// -- Draw Blinking Cursor if Focused --
		auto now = std::chrono::steady_clock::now();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

		if (hasFocus && (millis / 500) % 2 == 0) {
			Geometry cursorGeo = { {cursorX, allocatedGeometry.position.y + 5.0f}, {2.0f, 20.0f} };
			addRectToDrawList(outDrawList, cursorGeo, m_textColor);
		}

		// -- Pop the Clipping Rect --
		outDrawList.popClipRect();
	}

	EventReply SEditableText::onMouseButtonDown(const Geometry & allotedGeometry, const Vec2 & mousePos) {
		if (allotedGeometry.contains(mousePos)) {
			SWidget::setFocusedWidget(this);
			return EventReply::handled();
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
			if (!m_text.empty()) m_text.pop_back();
		}
		else if (c >= 32 && c <= 126) {
			m_text += c;
		}
		return EventReply::handled();
	}

	EventReply SEditableText::onKeyDown(int key) {
		return EventReply::handled();
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
