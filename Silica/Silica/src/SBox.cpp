#include "SBox.h"

#include "Theme.h"

namespace Silica {

	void SBox::construct(const Args& args) {
		m_padding = args.padding;
		m_backgroundColor = args.backgroundColor.value_or(GetTheme().backgroundPanel);
		m_hoverColor = args.hoverColor.value_or(Color(0, 0, 0, 0));
		m_onClick = args.onClick;
		m_child = args.child;
	}

	void SBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}

		m_desiredSize.x += m_padding.x * 2;
		m_desiredSize.y += m_padding.y * 2;
	}

	void SBox::arrangeChildren(const Geometry& allocatedGeometry) {
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

	void SBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		Color drawColor = (m_isHovered && m_hoverColor.a() > 0) ? m_hoverColor : m_backgroundColor;

		if (drawColor.a() > 0) {
			addRectToDrawList(outDrawList, allocatedGeometry, drawColor);
		}

		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			m_child->onDraw(outDrawList, childGeo);
		}
	}

	EventReply SBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onMouseMove(childGeo, mousePos);
			if (reply.isHandled) {
				m_isHovered = false;
				return reply;
			}
		}

		m_isHovered = allocatedGeometry.contains(mousePos);
		return m_isHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onMouseButtonDown(childGeo, mousePos);
			if (reply.isHandled) return reply;
		}

		if (allocatedGeometry.contains(mousePos)) {
			if (m_onClick) {
				return m_onClick();
			}
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onMouseButtonUp(childGeo, mousePos);
			if (reply.isHandled) return reply;
		}

		return EventReply::unhandled();
	}

	EventReply SBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			return m_child->onMouseWheel(childGeo, mousePos, scrollDelta);
		}
		return EventReply::unhandled();
	}

	void SBox::addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0.0f, 0.0f}, color }); // TL
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {0.0f, 0.0f}, color }); // TR
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BR
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0.0f, 0.0f}, color }); // BL

		drawList.indices.push_back(startIndex + 0);
		drawList.indices.push_back(startIndex + 1);
		drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 0);
		drawList.indices.push_back(startIndex + 2);
		drawList.indices.push_back(startIndex + 3);

		if (drawList.commands.empty()) {
			drawList.commands.push_back({ 0, 0, 0 });
		}
		drawList.commands.back().indexCount += 6;
	}

}
