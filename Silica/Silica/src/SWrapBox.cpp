#include "SWrapBox.h"

namespace Silica {

	void SWrapBox::construct(const Args& args) {
		m_spacing = args.spacing;
		m_children = args.children;
	}

	void SWrapBox::computeDesiredSize() {
		m_desiredSize = { 0.0f, 0.0f };
		for (auto& child : m_children) {
			if (child) child->computeDesiredSize();
		}
	}

	void SWrapBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		float currentX = m_spacing;
		float currentY = m_spacing;
		float rowHeight = 0.0f;

		for (auto& child : m_children) {
			if (!child) continue;
			Vec2 size = child->getDesiredSize();
			if (currentX + size.x > allocatedGeometry.size.x && currentX > m_spacing) {
				currentX = m_spacing;
				currentY += rowHeight + m_spacing;
				rowHeight = 0.0f;
			}
			child->arrangeChildren({ allocatedGeometry.position + Vec2(currentX, currentY), size });
			currentX += size.x + m_spacing;
			rowHeight = std::max(rowHeight, size.y);
		}
		m_desiredSize.y = currentY + rowHeight + m_spacing;
	}

	void SWrapBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		for (auto& child : m_children) {
			if (child) child->onDraw(outDrawList, child->getAllocatedGeometry());
		}
	}

	EventReply SWrapBox::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		for (auto& child : m_children) {
			if (child && child->onMouseMove(child->getAllocatedGeometry(), mousePos).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SWrapBox::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (auto& child : m_children) {
			if (child && child->onMouseButtonDown(child->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SWrapBox::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (auto& child : m_children) {
			if (child && child->onMouseButtonUp(child->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	EventReply SWrapBox::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (auto& child : m_children) {
			if (child && child->onMouseWheel(child->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
		}

		return EventReply::unhandled();
	}

	void SWrapBox::addChild(WidgetPtr child) {
		m_children.push_back(child);
	}

}
