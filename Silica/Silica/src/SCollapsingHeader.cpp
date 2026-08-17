#include "SCollapsingHeader.h"

#include "Theme.h"

namespace Silica {

	void SCollapsingHeader::construct(const Args& args) {
		m_title = args.title;
		m_isOpen = args.initiallyOpen;
		m_content = args.content;
		m_font = args.font ? args.font : GetTheme().Font_Default;
		m_trailingWidget = args.trailingWidget;

		m_headerColor = args.headerColor.value_or(GetTheme().Element_Normal);
		m_headerHoverColor = args.headerHoverColor.value_or(GetTheme().Element_Hover);
		m_textColor = args.textColor.value_or(GetTheme().Text_Main);
	}

	void SCollapsingHeader::computeDesiredSize() {
		m_desiredSize = Vec2(0.0f, m_headerHeight);

		if (m_trailingWidget) {
			m_trailingWidget->computeDesiredSize();
		}

		if (m_isOpen && m_content) {
			m_content->computeDesiredSize();
			Vec2 childSize = m_content->getDesiredSize();
			m_desiredSize.y += childSize.y;
			m_desiredSize.x = std::max(m_desiredSize.x, childSize.x);
		}
	}

	void SCollapsingHeader::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float currentHeaderHeight = m_headerHeight * m_renderScale;

		if (m_trailingWidget) {
			Vec2 trailingSize = m_trailingWidget->getDesiredSize();
			Vec2 scaledTrailingSize = { trailingSize.x * m_renderScale, trailingSize.y * m_renderScale };

			Geometry trailGeo;
			trailGeo.size = scaledTrailingSize;
			trailGeo.position.x = allocatedGeometry.position.x + allocatedGeometry.size.x - scaledTrailingSize.x - (4.0f * m_renderScale);
			trailGeo.position.y = allocatedGeometry.position.y + ((currentHeaderHeight - scaledTrailingSize.y) * 0.5f);
			m_trailingWidget->arrangeChildren(trailGeo);
		}

		if (m_isOpen && m_content) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x;
			childGeo.position.y = allocatedGeometry.position.y + currentHeaderHeight;
			childGeo.size.x = allocatedGeometry.size.x;
			childGeo.size.y = std::max(0.0f, allocatedGeometry.size.y - currentHeaderHeight);

			m_content->arrangeChildren(childGeo);
		}
	}

	void SCollapsingHeader::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		float currentHeaderHeight = m_headerHeight * m_renderScale;

		// -- Draw Header Background --
		Rect headerRect = getHeaderRect();
		Geometry headerGeo = { {headerRect.left, headerRect.top}, {headerRect.getWidth(), headerRect.getHeight()} };
		Color bgColor = m_isHeaderHovered ? m_headerHoverColor : m_headerColor;
		outDrawList.addRect(headerGeo, bgColor);

		// -- Draw Expand / Collapse Triangle --
		Vec2 triangleCenter(headerGeo.position.x + (12.0f * m_renderScale), headerGeo.position.y + (currentHeaderHeight * 0.5f));
		drawTriangle(outDrawList, triangleCenter, 5.0f * m_renderScale, m_isOpen, m_textColor);

		// -- Draw Header Title Text --
		if (m_font && !m_title.empty()) {
			Vec2 textPos = { headerGeo.position.x + (24.0f * m_renderScale), headerGeo.position.y + (16.0f * m_renderScale) };
			outDrawList.addText(m_font, m_title, textPos, m_textColor, m_renderScale);
		}

		if (m_trailingWidget) {
			m_trailingWidget->onDraw(outDrawList, m_trailingWidget->getAllocatedGeometry());
		}

		if (m_isOpen && m_content) {
			m_content->onDraw(outDrawList, m_content->getAllocatedGeometry());
		}
	}

	void SCollapsingHeader::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_content) m_content->setRenderScale(scale);
		if (m_trailingWidget) m_trailingWidget->setRenderScale(scale);
	}

	EventReply SCollapsingHeader::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		m_isHeaderHovered = getHeaderRect().contains(mousePos);
		EventReply finalReply = EventReply::unhandled();

		if (m_trailingWidget) {
			EventReply reply = m_trailingWidget->onMouseMove(m_trailingWidget->getAllocatedGeometry(), mousePos);
			if (reply.isHandled) finalReply = reply;
		}

		if (m_isOpen && m_content) {
			EventReply reply = m_content->onMouseMove(m_content->getAllocatedGeometry(), mousePos);
			if (reply.isHandled) finalReply = reply;
		}

		if (finalReply.isHandled) {
			m_isHeaderHovered = false;
			return EventReply::handled();
		}

		return m_isHeaderHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (button != MouseButton::Left) return EventReply::unhandled();

		if (m_trailingWidget && m_trailingWidget->getAllocatedGeometry().contains(mousePos)) {
			return m_trailingWidget->onMouseButtonDown(m_trailingWidget->getAllocatedGeometry(), mousePos, button);
		}

		// -- Toggle State --
		if (getHeaderRect().contains(mousePos)) {
			m_isOpen = !m_isOpen;
			return EventReply::handled();
		}

		// -- Pass To Content --
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonDown(m_content->getAllocatedGeometry(), mousePos, button);
		}

		return EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {

		if (m_trailingWidget && m_trailingWidget->getAllocatedGeometry().contains(mousePos)) {
			return m_trailingWidget->onMouseButtonUp(m_trailingWidget->getAllocatedGeometry(), mousePos, button);
		}

		// -- Pass To Content --
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseButtonUp(m_content->getAllocatedGeometry(), mousePos, button);
		}

		return EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {

		if (m_trailingWidget && m_trailingWidget->getAllocatedGeometry().contains(mousePos)) {
			return m_trailingWidget->onMouseWheel(m_trailingWidget->getAllocatedGeometry(), mousePos, scrollDelta);
		}

		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			return m_content->onMouseWheel(m_content->getAllocatedGeometry(), mousePos, scrollDelta);
		}

		return EventReply::unhandled();
	}

	Rect SCollapsingHeader::getHeaderRect() const {
		float currentHeaderHeight = m_headerHeight * m_renderScale;
		return Rect(
			m_allocatedGeometry.position.x,
			m_allocatedGeometry.position.x + m_allocatedGeometry.size.x,
			m_allocatedGeometry.position.y,
			m_allocatedGeometry.position.y + currentHeaderHeight
		);
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

	EventReply SCollapsingHeader::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		// -- Trailing Widget --
		if (m_trailingWidget && m_trailingWidget->getAllocatedGeometry().contains(mousePos)) {
			EventReply reply = m_trailingWidget->onDragOver(m_trailingWidget->getAllocatedGeometry(), mousePos, payload);
			if (reply.isHandled) return reply;
		}

		// -- Main Content If Header Is Expanded --
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			EventReply reply = m_content->onDragOver(m_content->getAllocatedGeometry(), mousePos, payload);
			if (reply.isHandled) return reply;
		}

		return EventReply::unhandled();
	}

	EventReply SCollapsingHeader::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		// -- Trailing Widget --
		if (m_trailingWidget && m_trailingWidget->getAllocatedGeometry().contains(mousePos)) {
			EventReply reply = m_trailingWidget->onDrop(m_trailingWidget->getAllocatedGeometry(), mousePos, payload);
			if (reply.isHandled) return reply;
		}

		// -- Main Content If Header Is Expanded --
		if (m_isOpen && m_content && m_content->getAllocatedGeometry().contains(mousePos)) {
			EventReply reply = m_content->onDrop(m_content->getAllocatedGeometry(), mousePos, payload);
			if (reply.isHandled) return reply;
		}

		return EventReply::unhandled();
	}

}
