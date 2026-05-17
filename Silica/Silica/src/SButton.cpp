#include "SButton.h"

namespace Silica {

	void SButton::construct(const Args& args) {
		m_padding = args.padding;
		m_color = args.color;
		m_hoverColor = args.hoverColor;
		m_pressedColor = args.pressedColor;
		m_onClick = args.onClick;
		m_child = args.child;
	}

	void SButton::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}
		m_desiredSize.x += m_padding.x * 2;
		m_desiredSize.y += m_padding.y * 2;
	}

	void SButton::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);
			m_child->arrangeChildren(childGeo);
		}
	}

	void SButton::onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const {
		Color drawColor = m_isPressed ? m_pressedColor : (m_isHovered ? m_hoverColor : m_color);

		if (drawColor.a() > 0) {
			addRectToDrawList(outDrawList, allotedGeometry, drawColor);
		}

		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allotedGeometry.position.x + m_padding.x;
			childGeo.position.y = allotedGeometry.position.y + m_padding.y;
			childGeo.size.x = allotedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allotedGeometry.size.y - (m_padding.y * 2.0f);
			m_child->onDraw(outDrawList, childGeo);
		}
	}

	EventReply SButton::onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) {
		m_isHovered = allotedGeometry.contains(mousePos);

		if (!m_isHovered) {
			m_isPressed = false;
		}

		if (m_child) {
			// We don't bother recalculating child geo here for brevity,
			// but in a full system you would pass it down just like SBox does.
		}

		return m_isHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SButton::onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (allotedGeometry.contains(mousePos)) {
			m_isPressed = true;
			return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SButton::onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_isPressed && allotedGeometry.contains(mousePos)) {
			m_isPressed = false;
			if (m_onClick) {
				return m_onClick();
			}
			return EventReply::handled();
		}

		m_isPressed = false;
		return EventReply::unhandled();
	}

	void SButton::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, color }); // TL
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, color }); // TR
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BR
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BL

		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 1); drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 0); drawList.indices.push_back(startIndex + 2); drawList.indices.push_back(startIndex + 3);

		if (drawList.commands.empty()) {
			drawList.commands.push_back({ 0, 0, 0 });
		}
		drawList.commands.back().indexCount += 6;
	}

}
