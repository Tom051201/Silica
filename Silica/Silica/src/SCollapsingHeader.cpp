#include "SCollapsingHeader.h"
#include "Theme.h"

namespace Silica {

	void SCollapsingHeader::construct(const Args& args) {
		m_title = args.title;
		m_isOpen = args.initiallyOpen;
		m_content = args.content;
		m_font = args.font;

		m_headerColor = args.headerColor.value_or(Color(50, 50, 50, 255));
		m_headerHoverColor = args.headerHoverColor.value_or(Color(70, 70, 70, 255));
		m_textColor = args.textColor.value_or(GetTheme().textMain);
	}

	void SCollapsingHeader::computeDesiredSize() {
		m_desiredSize = Vec2(0.0f, m_headerHeight);

		if (m_isOpen && m_content) {
			m_content->computeDesiredSize();
			Vec2 childSize = m_content->getDesiredSize();
			m_desiredSize.y += childSize.y;
			m_desiredSize.x = childSize.x;
		}
	}

	void SCollapsingHeader::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_isOpen && m_content) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x;
			childGeo.position.y = allocatedGeometry.position.y + m_headerHeight;
			childGeo.size.x = allocatedGeometry.size.x;

			childGeo.size.y = allocatedGeometry.size.y - m_headerHeight;
			if (childGeo.size.y < 0) childGeo.size.y = 0;

			m_content->arrangeChildren(childGeo);
		}
	}

	void SCollapsingHeader::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw Header Background --
		Rect headerRect = getHeaderRect();
		Geometry headerGeo = { {headerRect.left, headerRect.top}, {headerRect.getWidth(), headerRect.getHeight()} };
		Color bgColor = m_isHeaderHovered ? m_headerHoverColor : m_headerColor;
		addRectToDrawList(outDrawList, headerGeo, bgColor);

		// -- Draw Expand / Collapse Triangle --
		Vec2 triangleCenter(headerGeo.position.x + 12.0f, headerGeo.position.y + (m_headerHeight * 0.5f));
		drawTriangle(outDrawList, triangleCenter, 5.0f, m_isOpen, m_textColor);

		// -- Draw Header Title Text --
		if (m_font && !m_title.empty()) {
			float cursorX = headerGeo.position.x + 24.0f;
			float baselineY = headerGeo.position.y + 16.0f;

			for (char c : m_title) {
				const Glyph& g = m_font->getGlyph(c);
				if (g.size.x > 0 && g.size.y > 0) {
					float x0 = cursorX + g.offset.x;
					float y0 = baselineY + g.offset.y;
					float x1 = x0 + g.size.x;
					float y1 = y0 + g.size.y;

					uint32_t startIndex = (uint32_t)outDrawList.vertices.size();
					outDrawList.vertices.push_back({ {x0, y0}, {g.uvMin.x, g.uvMin.y}, m_textColor });
					outDrawList.vertices.push_back({ {x1, y0}, {g.uvMax.x, g.uvMin.y}, m_textColor });
					outDrawList.vertices.push_back({ {x1, y1}, {g.uvMax.x, g.uvMax.y}, m_textColor });
					outDrawList.vertices.push_back({ {x0, y1}, {g.uvMin.x, g.uvMax.y}, m_textColor });

					outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 1); outDrawList.indices.push_back(startIndex + 2);
					outDrawList.indices.push_back(startIndex + 0); outDrawList.indices.push_back(startIndex + 2); outDrawList.indices.push_back(startIndex + 3);
					if (outDrawList.commands.empty()) outDrawList.commands.push_back({ 0, 0, 0 });
					outDrawList.commands.back().indexCount += 6;
				}
				cursorX += g.advanceX;
			}
		}

		// -- Draw Content If Open --
		if (m_isOpen && m_content) {
			m_content->onDraw(outDrawList, m_content->getAllocatedGeometry());
		}
	}

	EventReply SCollapsingHeader::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		m_isHeaderHovered = getHeaderRect().contains(mousePos);

		if (m_isOpen && m_content) {
			EventReply reply = m_content->onMouseMove(m_content->getAllocatedGeometry(), mousePos);
			if (reply.isHandled) return reply;
		}

		return m_isHeaderHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		// -- Toggle State --
		if (getHeaderRect().contains(mousePos)) {
			m_isOpen = !m_isOpen;
			return EventReply::handled();
		}

		// -- Pass To Content --
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonDown(m_content->getAllocatedGeometry(), mousePos);
		}

		return EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonUp(m_content->getAllocatedGeometry(), mousePos);
		}

		return EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseWheel(m_content->getAllocatedGeometry(), mousePos, scrollDelta);
		}

		return EventReply::unhandled();
	}

	Rect SCollapsingHeader::getHeaderRect() const {
		return Rect(
			m_allocatedGeometry.position.x,
			m_allocatedGeometry.position.x + m_allocatedGeometry.size.x,
			m_allocatedGeometry.position.y,
			m_allocatedGeometry.position.y + m_headerHeight
		);
	}

	void SCollapsingHeader::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
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

	void SCollapsingHeader::drawTriangle(DrawList& drawList, const Vec2& center, float radius, bool isOpen, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		Vec2 p0, p1, p2;

		if (isOpen) {
			// -- Pointing Down --
			p0 = { center.x - radius,	center.y - (radius * 0.5f) };
			p1 = { center.x + radius,	center.y - (radius * 0.5f) };
			p2 = { center.x,			center.y + (radius * 0.5f) };
		}
		else {
			// -- Pointing Right --
			p0 = { center.x - (radius * 0.5f), center.y - radius };
			p1 = { center.x - (radius * 0.5f), center.y + radius };
			p2 = { center.x + (radius * 0.5f), center.y };
		}

		drawList.vertices.push_back({ p0, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ p1, {0.0f, 0.0f}, color });
		drawList.vertices.push_back({ p2, {0.0f, 0.0f}, color });

		drawList.indices.push_back(startIndex + 0);
		drawList.indices.push_back(startIndex + 1);
		drawList.indices.push_back(startIndex + 2);

		if (drawList.commands.empty()) drawList.commands.push_back({ 0, 0, 0 });
		drawList.commands.back().indexCount += 3;
	}

}
