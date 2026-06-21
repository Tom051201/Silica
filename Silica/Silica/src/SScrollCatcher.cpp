#include "SScrollCatcher.h"

namespace Silica {

	void SScrollCatcher::construct(const Args& args) {
		m_onMouseWheel = args.onMouseWheel;
		m_child = args.child;
	}

	void SScrollCatcher::computeDesiredSize()  {
		if (m_child) m_child->computeDesiredSize(); m_desiredSize = m_child->getDesiredSize();
	}

	void SScrollCatcher::arrangeChildren(const Geometry& geom) {
		SWidget::arrangeChildren(geom);
		if (m_child) m_child->arrangeChildren(geom);
	}

	void SScrollCatcher::onDraw(DrawList& drawList, const Geometry& geom) const  {
		if (m_child) m_child->onDraw(drawList, m_child->getAllocatedGeometry());
	}

	EventReply SScrollCatcher::onMouseMove(const Geometry& geom, const Vec2& pos)  {
		if (m_child) return m_child->onMouseMove(m_child->getAllocatedGeometry(), pos);

		return EventReply::unhandled();
	}

	EventReply SScrollCatcher::onMouseButtonDown(const Geometry& geom, const Vec2& pos, MouseButton btn)  {
		if (m_child) return m_child->onMouseButtonDown(m_child->getAllocatedGeometry(), pos, btn);

		return EventReply::unhandled();
	}

	EventReply SScrollCatcher::onMouseButtonUp(const Geometry& geom, const Vec2& pos, MouseButton btn)  {
		if (m_child) return m_child->onMouseButtonUp(m_child->getAllocatedGeometry(), pos, btn);

		return EventReply::unhandled();
	}

	EventReply SScrollCatcher::onMouseWheel(const Geometry& geom, const Vec2& pos, float delta)  {
		if (m_onMouseWheel) {
			auto reply = m_onMouseWheel(delta);
			if (reply.isHandled) return reply;
		}
		if (m_child) return m_child->onMouseWheel(m_child->getAllocatedGeometry(), pos, delta);

		return EventReply::unhandled();
	}

}
