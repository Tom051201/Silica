#include "SBox.h"

namespace Silica {

	void SBox::construct(const Args& args) {
		m_padding = args.padding;
		m_backgroundColor = args.backgroundColor;
		m_hoverColor = args.hoverColor;
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
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allocatedGeometry.position.x + m_padding.x;
			childGeo.position.y = allocatedGeometry.position.y + m_padding.y;
			childGeo.size.x = allocatedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allocatedGeometry.size.y - (m_padding.y * 2.0f);

			m_child->arrangeChildren(childGeo);
		}
	}

	void SBox::onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const {
		uint32_t drawColor = m_isHovered && (m_hoverColor >> 24) > 0 ? m_hoverColor : m_backgroundColor;

		if ((drawColor >> 24) > 0) {
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

	EventReply SBox::onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allotedGeometry.position.x + m_padding.x;
			childGeo.position.y = allotedGeometry.position.y + m_padding.y;
			childGeo.size.x = allotedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allotedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onMouseMove(childGeo, mousePos);
			if (reply.isHandled) {
				m_isHovered = false;
				return reply;
			}
		}

		m_isHovered = allotedGeometry.contains(mousePos);

		return m_isHovered ? EventReply::handled() : EventReply::unhandled();
	}

	EventReply SBox::onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allotedGeometry.position.x + m_padding.x;
			childGeo.position.y = allotedGeometry.position.y + m_padding.y;
			childGeo.size.x = allotedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allotedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onMouseButtonDown(childGeo, mousePos);
			if (reply.isHandled) return reply;
		}

		if (allotedGeometry.contains(mousePos)) {
			if (m_onClick) {
				return m_onClick();
			}
			return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SBox::onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_child) {
			Geometry childGeo;
			childGeo.position.x = allotedGeometry.position.x + m_padding.x;
			childGeo.position.y = allotedGeometry.position.y + m_padding.y;
			childGeo.size.x = allotedGeometry.size.x - (m_padding.x * 2.0f);
			childGeo.size.y = allotedGeometry.size.y - (m_padding.y * 2.0f);

			EventReply reply = m_child->onMouseButtonUp(childGeo, mousePos);
			if (reply.isHandled) return reply;
		}

		return EventReply::unhandled();
	}

	void SBox::addRectToDrawList(DrawList& drawList, const Geometry& geo, uint32_t color) const {
		uint32_t startIndex = (uint32_t)drawList.vertices.size();

		drawList.vertices.push_back({ {geo.position.x, geo.position.y}, {0,0}, color }); // TL
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y}, {1,0}, color }); // TR
		drawList.vertices.push_back({ {geo.position.x + geo.size.x, geo.position.y + geo.size.y}, {1,1}, color }); // BR
		drawList.vertices.push_back({ {geo.position.x, geo.position.y + geo.size.y}, {0,1}, color }); // BL

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
