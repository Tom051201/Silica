#include "SScissorBox.h"

namespace Silica {

	void SScissorBox::computeDesiredSize() {
		m_desiredSize = Vec2::zero();
		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}
	}

	void SScissorBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		if (m_child) {
			m_child->arrangeChildren(allocatedGeometry);
		}
	}

	void SScissorBox::onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const {
		if (m_child) {
			Rect myRect(
				allotedGeometry.position.x,
				allotedGeometry.position.x + allotedGeometry.size.x,
				allotedGeometry.position.y,
				allotedGeometry.position.y + allotedGeometry.size.y
			);

			outDrawList.pushClipRect(myRect);

			m_child->onDraw(outDrawList, m_child->getAllocatedGeometry());

			outDrawList.popClipRect();
		}
	}

	EventReply SScissorBox::onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_child && allotedGeometry.contains(mousePos)) {
			return m_child->onMouseMove(m_child->getAllocatedGeometry(), mousePos);
		}
		return EventReply::unhandled();
	}

	EventReply SScissorBox::onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_child && allotedGeometry.contains(mousePos)) {
			return m_child->onMouseButtonDown(m_child->getAllocatedGeometry(), mousePos);
		}
		return EventReply::unhandled();
	}

	EventReply SScissorBox::onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) {
		if (m_child && allotedGeometry.contains(mousePos)) {
			return m_child->onMouseButtonUp(m_child->getAllocatedGeometry(), mousePos);
		}
		return EventReply::unhandled();
	}
}