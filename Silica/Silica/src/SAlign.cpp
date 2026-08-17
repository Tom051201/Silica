#include "SAlign.h"

namespace Silica {

	void SAlign::construct(const Args& args) {
		m_horizontalAlign = args.horizontalAlign;
		m_verticalAlign = args.verticalAlign;
		m_child = args.child;
	}

	void SAlign::computeDesiredSize() {
		if (m_child) {
			m_child->computeDesiredSize();
			m_desiredSize = m_child->getDesiredSize();
		}
		else {
			m_desiredSize = Vec2::zero();
		}
	}

	void SAlign::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		if (!m_child) return;

		Geometry childGeo = allocatedGeometry;
		Vec2 childDesired = m_child->getDesiredSize();

		// -- Horizontal Math --
		if (m_horizontalAlign != HorizontalAlign::Fill) {
			childGeo.size.x = childDesired.x;

			if (m_horizontalAlign == HorizontalAlign::Center) {
				childGeo.position.x += (allocatedGeometry.size.x - childDesired.x) * 0.5f;
			}
			else if (m_horizontalAlign == HorizontalAlign::Right) {
				childGeo.position.x += (allocatedGeometry.size.x - childDesired.x);
			}
		}

		// -- Vertical Math --
		if (m_verticalAlign != VerticalAlign::Fill) {
			childGeo.size.y = childDesired.y;

			if (m_verticalAlign == VerticalAlign::Center) {
				childGeo.position.y += (allocatedGeometry.size.y - childDesired.y) * 0.5f;
			}
			else if (m_verticalAlign == VerticalAlign::Bottom) {
				childGeo.position.y += (allocatedGeometry.size.y - childDesired.y);
			}
		}

		m_child->arrangeChildren(childGeo);
	}

	void SAlign::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_child) m_child->onDraw(outDrawList, m_child->getAllocatedGeometry());
	}

	void SAlign::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_child) m_child->setRenderScale(scale);
	}

	EventReply SAlign::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_child) return m_child->onMouseMove(m_child->getAllocatedGeometry(), mousePos);
		return EventReply::unhandled();
	}

	EventReply SAlign::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_child) return m_child->onMouseButtonDown(m_child->getAllocatedGeometry(), mousePos, button);
		return EventReply::unhandled();
	}

	EventReply SAlign::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_child) return m_child->onMouseButtonUp(m_child->getAllocatedGeometry(), mousePos, button);
		return EventReply::unhandled();
	}

	EventReply SAlign::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_child) return m_child->onMouseWheel(m_child->getAllocatedGeometry(), mousePos, scrollDelta);
		return EventReply::unhandled();
	}

	EventReply SAlign::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child) return m_child->onDragOver(m_child->getAllocatedGeometry(), mousePos, payload);
		return EventReply::unhandled();
	}

	EventReply SAlign::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child) return m_child->onDrop(m_child->getAllocatedGeometry(), mousePos, payload);
		return EventReply::unhandled();
	}

}
