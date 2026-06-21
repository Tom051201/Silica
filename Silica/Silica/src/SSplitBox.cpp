#include "SSplitBox.h"

namespace Silica {

	void SSplitBox::construct(const Args& args) {
		m_leftWidth = args.leftWidth;
		m_left = args.leftContent;
		m_right = args.rightContent;
	}

	void SSplitBox::computeDesiredSize()  {
		m_desiredSize = { 0,0 };
		if (m_left) {
			m_left->computeDesiredSize();
			m_desiredSize.x += m_leftWidth;
			m_desiredSize.y = std::max(m_desiredSize.y, m_left->getDesiredSize().y);
		}
		if (m_right) {
			m_right->computeDesiredSize();
			m_desiredSize.x += m_right->getDesiredSize().x;
			m_desiredSize.y = std::max(m_desiredSize.y, m_right->getDesiredSize().y);
		}
	}

	void SSplitBox::arrangeChildren(const Geometry& allocatedGeometry)  {
		SWidget::arrangeChildren(allocatedGeometry);

		if (m_left) {
			m_left->arrangeChildren({ allocatedGeometry.position, {m_leftWidth, allocatedGeometry.size.y} });
		}

		if (m_right) {
			float rightWidth = std::max(0.0f, allocatedGeometry.size.x - m_leftWidth);
			m_right->arrangeChildren({
				{allocatedGeometry.position.x + m_leftWidth, allocatedGeometry.position.y},
				{rightWidth, allocatedGeometry.size.y}
			});
		}
	}

	void SSplitBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const  {
		if (m_left) m_left->onDraw(outDrawList, m_left->getAllocatedGeometry());
		if (m_right) m_right->onDraw(outDrawList, m_right->getAllocatedGeometry());
	}

	EventReply SSplitBox::onMouseMove(const Geometry& geo, const Vec2& pos)  {
		if (m_left && m_left->onMouseMove(m_left->getAllocatedGeometry(), pos).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseMove(m_right->getAllocatedGeometry(), pos).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}
	EventReply SSplitBox::onMouseButtonDown(const Geometry& geo, const Vec2& pos, MouseButton btn)  {
		if (m_left && m_left->onMouseButtonDown(m_left->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseButtonDown(m_right->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}
	EventReply SSplitBox::onMouseButtonUp(const Geometry& geo, const Vec2& pos, MouseButton btn)  {
		if (m_left && m_left->onMouseButtonUp(m_left->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseButtonUp(m_right->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}
	EventReply SSplitBox::onMouseWheel(const Geometry& geo, const Vec2& pos, float delta)  {
		if (m_left && m_left->onMouseWheel(m_left->getAllocatedGeometry(), pos, delta).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseWheel(m_right->getAllocatedGeometry(), pos, delta).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

}
