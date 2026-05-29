#include "SOverlay.h"

#include <algorithm>

namespace Silica {

	void SOverlay::construct(const Args& args) {
		m_children = args.children;
	}

	void SOverlay::computeDesiredSize() {
		m_desiredSize = Vec2::zero();

		for (auto& child : m_children) {
			if (child) {
				child->computeDesiredSize();
			}
		}
	}

	void SOverlay::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
		for (auto& child : m_children) {
			if (child) child->arrangeChildren(allocatedGeometry);
		}
	}

	void SOverlay::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		for (auto& child : m_children) {
			if (child) child->onDraw(outDrawList, child->getAllocatedGeometry());
		}
	}

	EventReply SOverlay::onMouseButtonDown(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (auto it = m_children.rbegin(); it != m_children.rend(); it++) {
			if (*it && (*it)->onMouseButtonDown((*it)->getAllocatedGeometry(), mousePos, button).isHandled) {
				return EventReply::handled();
			}
		}
		return EventReply::unhandled();
	}

	EventReply SOverlay::onMouseMove(const Geometry& allocatedGeometry, const Vec2& mousePos) {
		EventReply finalReply = EventReply::unhandled();
		for (auto it = m_children.rbegin(); it != m_children.rend(); it++) {
			if (*it) {
				EventReply reply = (*it)->onMouseMove((*it)->getAllocatedGeometry(), mousePos);
				if (reply.isHandled) finalReply = EventReply::handled();
			}
		}
		return finalReply;
	}

	EventReply SOverlay::onMouseButtonUp(const Geometry& allocatedGeometry, const Vec2& mousePos, MouseButton button) {
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
			if (*it && (*it)->onMouseButtonUp((*it)->getAllocatedGeometry(), mousePos, button).isHandled) return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	EventReply SOverlay::onMouseWheel(const Geometry& allocatedGeometry, const Vec2& mousePos, float scrollDelta) {
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
			if (*it && (*it)->onMouseWheel((*it)->getAllocatedGeometry(), mousePos, scrollDelta).isHandled) return EventReply::handled();
		}
		return EventReply::unhandled();
	}

	void SOverlay::addChild(WidgetPtr child) {
		m_children.push_back(child);
	}

	void SOverlay::removeChild(WidgetPtr child) {
		auto it = std::find(m_children.begin(), m_children.end(), child);
		if (it != m_children.end()) {
			m_children.erase(it);
		}
	}

}
