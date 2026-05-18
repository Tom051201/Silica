#include "SWindow.h"

#include "Renderer.h"
#include "Theme.h"

namespace Silica {

	void SWindow::construct(const Args& args) {
		m_title = args.title;
		m_position = args.initialPosition;
		m_size = args.initialSize;
		m_content = args.content;
		m_font = args.font;
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().backgroundWindow);
		m_titleBarColor = args.titleBarColor.value_or(GetTheme().backgroundPanel);
		m_titleBarDraggingColor = args.titleBarDraggingColor.value_or(GetTheme().accentPrimary);
		m_titleTextColor = args.titleTextColor.value_or(GetTheme().textMain);
	}

	void SWindow::computeDesiredSize() {
		m_desiredSize = m_size;
		if (m_content) m_content->computeDesiredSize();
	}

	void SWindow::arrangeChildren(const Geometry & allocatedGeometry) {
		m_allocatedGeometry.position = m_position;
		m_allocatedGeometry.size = m_size;

		if (m_content) {
			Geometry contentGeo;
			contentGeo.position.x = m_position.x;
			contentGeo.position.y = m_position.y + 30.0f;
			contentGeo.size.x = m_size.x;
			contentGeo.size.y = m_size.y - 30.0f;
			m_content->arrangeChildren(contentGeo);
		}
	}

	void SWindow::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw Window Background --
		addRectToDrawList(outDrawList, m_allocatedGeometry, m_backgroundColor);

		// -- Draw Title Bar --
		Color titleColor = m_isDragging ? m_titleBarDraggingColor : m_titleBarColor;
		addRectToDrawList(outDrawList, { m_position, {m_size.x, 30.0f} }, titleColor);

		// -- Draw Title Text --
		if (m_font && !m_title.empty()) {
			float cursorX = m_position.x + 10.0f;
			float baselineY = m_position.y + 20.0f;
			for (char c : m_title) {
				const Glyph& g = m_font->getGlyph(c);
				if (g.size.x > 0 && g.size.y > 0) {
					float x0 = cursorX + g.offset.x;
					float y0 = baselineY + g.offset.y;
					float x1 = x0 + g.size.x;
					float y1 = y0 + g.size.y;

					uint32_t startIndex = (uint32_t)outDrawList.vertices.size();
					outDrawList.vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, m_titleTextColor });
					outDrawList.vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, m_titleTextColor });
					outDrawList.vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, m_titleTextColor });
					outDrawList.vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, m_titleTextColor });

					outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 1); outDrawList.indices.push_back(startIndex + 2);
					outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 2); outDrawList.indices.push_back(startIndex + 3);
					if (outDrawList.commands.empty()) outDrawList.commands.push_back({ 0, 0, 0 });
					outDrawList.commands.back().indexCount += 6;
				}
				cursorX += g.advanceX;
			}
		}

		// -- Draw Content --
		if (m_content) {
			outDrawList.pushClipRect(Rect(m_position.x, m_position.x + m_size.x, m_position.y + 30.0f, m_position.y + m_size.y));
			m_content->onDraw(outDrawList, m_content->getAllocatedGeometry());
			outDrawList.popClipRect();
		}
	}

	EventReply SWindow::onMouseMove(const Geometry & allocatedGeometry, const Vec2 & mousePos) {
		if (m_isDragging) {
			m_position.x = mousePos.x - m_dragClickOffset.x;
			m_position.y = mousePos.y - m_dragClickOffset.y;
			return EventReply::handled();
		}

		if (m_content) {
			EventReply reply = m_content->onMouseMove(m_content->getAllocatedGeometry(), mousePos);
			if (reply.isHandled) return reply;
		}

		if (m_allocatedGeometry.contains(mousePos)) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SWindow::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (getTitleBarRect().contains(mousePos)) {
			m_isDragging = true;
			m_dragClickOffset = Vec2(mousePos.x - m_position.x, mousePos.y - m_position.y);
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonDown(m_content->getAllocatedGeometry(), mousePos);
		}

		if (m_allocatedGeometry.contains(mousePos)) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SWindow::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isDragging) {
			m_isDragging = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		if (m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonUp(m_content->getAllocatedGeometry(), mousePos);
		}
		return EventReply::unhandled();
	}

	Rect SWindow::getTitleBarRect() const {
		return Rect(m_position.x, m_position.x + m_size.x, m_position.y, m_position.y + 30.0f);
	}

	void SWindow::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
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
