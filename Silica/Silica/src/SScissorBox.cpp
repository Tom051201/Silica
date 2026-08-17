#include "silicapch.h"
#include "SScissorBox.h"

namespace Silica {

	void SScissorBox::construct(const Args& args) {
		m_child = args.child;
	}

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

	void SScissorBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_child) {
			Rect myRect(
				allocatedGeometry.position.x,
				allocatedGeometry.position.x + allocatedGeometry.size.x,
				allocatedGeometry.position.y,
				allocatedGeometry.position.y + allocatedGeometry.size.y
			);

			outDrawList.pushClipRect(myRect);

			m_child->onDraw(outDrawList, m_child->getAllocatedGeometry());

			outDrawList.popClipRect();
		}
	}

	void SScissorBox::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_child) {
			m_child->setRenderScale(scale);
		}
	}

	EventReply SScissorBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		if (m_child) {
			if (allocatedGeometry.contains(mousePos)) {
				return m_child->onMouseMove(m_child->getAllocatedGeometry(), mousePos);
			}
			else {
				m_child->onMouseMove(m_child->getAllocatedGeometry(), Vec2(-9999.0f, -9999.0f));
			}
		}

		return EventReply::unhandled();
	}

	EventReply SScissorBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onMouseButtonDown(m_child->getAllocatedGeometry(), mousePos, button);
		}

		return EventReply::unhandled();
	}

	EventReply SScissorBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onMouseButtonUp(m_child->getAllocatedGeometry(), mousePos, button);
		}

		return EventReply::unhandled();
	}

	EventReply SScissorBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onMouseWheel(m_child->getAllocatedGeometry(), mousePos, scrollDelta);
		}

		return EventReply::unhandled();
	}

	EventReply SScissorBox::onDragOver(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onDragOver(m_child->getAllocatedGeometry(), mousePos, payload);
		}

		return EventReply::unhandled();
	}

	EventReply SScissorBox::onDrop(const Geometry& allocatedGeometry, const Vec2& mousePos, const DragDropPayload& payload) {
		if (m_child && allocatedGeometry.contains(mousePos)) {
			return m_child->onDrop(m_child->getAllocatedGeometry(), mousePos, payload);
		}

		return EventReply::unhandled();
	}

}
