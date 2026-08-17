#include "silicapch.h"
#include "SHorizontalSplitBox.h"

namespace Silica {

	void SHorizontalSplitBox::construct(const Args& args) {
		m_leftWidth = args.leftWidth;
		m_splitterThickness = args.splitterThickness;
		m_left = args.leftContent;
		m_right = args.rightContent;
	}

	void SHorizontalSplitBox::computeDesiredSize() {
		m_desiredSize = { 0, 0 };
		float scaledLeftWidth = m_leftWidth * m_renderScale;
		float scaledThickness = m_splitterThickness * m_renderScale;

		if (m_left) {
			m_left->computeDesiredSize();
			m_desiredSize.x += scaledLeftWidth;
			m_desiredSize.y = std::max(m_desiredSize.y, m_left->getDesiredSize().y);
		}

		m_desiredSize.x += scaledThickness;

		if (m_right) {
			m_right->computeDesiredSize();
			m_desiredSize.x += m_right->getDesiredSize().x;
			m_desiredSize.y = std::max(m_desiredSize.y, m_right->getDesiredSize().y);
		}
	}

	void SHorizontalSplitBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float scaledLeftWidth = m_leftWidth * m_renderScale;
		float scaledThickness = m_splitterThickness * m_renderScale;

		if (m_left) {
			m_left->arrangeChildren({ allocatedGeometry.position, {scaledLeftWidth, allocatedGeometry.size.y} });
		}

		if (m_right) {
			float rightX = allocatedGeometry.position.x + scaledLeftWidth + scaledThickness;
			float rightWidth = std::max(0.0f, allocatedGeometry.size.x - scaledLeftWidth - scaledThickness);

			m_right->arrangeChildren({
				{ rightX, allocatedGeometry.position.y },
				{ rightWidth, allocatedGeometry.size.y }
				});
		}
	}

	void SHorizontalSplitBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		// -- Draw Children --
		if (m_left) m_left->onDraw(outDrawList, m_left->getAllocatedGeometry());
		if (m_right) m_right->onDraw(outDrawList, m_right->getAllocatedGeometry());

		// -- Draw Splitter Bar --
		Rect sRect = getSplitterRect(allocatedGeometry);
		Geometry sGeo = { {sRect.left, sRect.top}, {sRect.right - sRect.left, sRect.bottom - sRect.top} };

		Color barColor = GetTheme().Surface_Tertiary;
		if (m_isDraggingSplitter) barColor = GetTheme().Accent_Primary;
		else if (m_isHoveredSplitter) barColor = GetTheme().Element_Hover;

		outDrawList.addRect(sGeo, barColor);
	}

	void SHorizontalSplitBox::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_left) m_left->setRenderScale(scale);
		if (m_right) m_right->setRenderScale(scale);
	}

	EventReply SHorizontalSplitBox::onMouseMove(const Geometry& geo, const Vec2& pos) {
		m_isHoveredSplitter = getSplitterRect(geo).contains(pos);

		// -- Handle Active Dragging --
		if (m_isDraggingSplitter) {
			float deltaX = (pos.x - m_dragStartX) / m_renderScale;
			m_leftWidth = std::max(100.0f, m_initialLeftWidth + deltaX);

			float maxLeftWidth = (geo.size.x / m_renderScale) - 100.0f;
			if (maxLeftWidth > 100.0f) {
				m_leftWidth = std::min(m_leftWidth, maxLeftWidth);
			}

			arrangeChildren(geo);
			return EventReply::handled();
		}

		if (m_isHoveredSplitter) {
			Platform::setCursor(Platform::Cursor::ResizeEW);
			return EventReply::handled();
		}

		if (m_left && m_left->onMouseMove(m_left->getAllocatedGeometry(), pos).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseMove(m_right->getAllocatedGeometry(), pos).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SHorizontalSplitBox::onMouseButtonDown(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		if (btn == MouseButton::Left && getSplitterRect(geo).contains(pos)) {
			m_isDraggingSplitter = true;
			m_dragStartX = pos.x;
			m_initialLeftWidth = m_leftWidth;
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		if (m_left && m_left->onMouseButtonDown(m_left->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseButtonDown(m_right->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SHorizontalSplitBox::onMouseButtonUp(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		if (btn == MouseButton::Left && m_isDraggingSplitter) {
			m_isDraggingSplitter = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		if (m_left && m_left->onMouseButtonUp(m_left->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseButtonUp(m_right->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SHorizontalSplitBox::onMouseWheel(const Geometry& geo, const Vec2& pos, float delta) {
		if (m_left && m_left->onMouseWheel(m_left->getAllocatedGeometry(), pos, delta).isHandled) return EventReply::handled();
		if (m_right && m_right->onMouseWheel(m_right->getAllocatedGeometry(), pos, delta).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SHorizontalSplitBox::onDragOver(const Geometry& geo, const Vec2& pos, const DragDropPayload& payload) {
		if (m_left && m_left->onDragOver(m_left->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		if (m_right && m_right->onDragOver(m_right->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SHorizontalSplitBox::onDrop(const Geometry& geo, const Vec2& pos, const DragDropPayload& payload) {
		if (m_left && m_left->onDrop(m_left->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		if (m_right && m_right->onDrop(m_right->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		return EventReply::unhandled();
	}

	Rect SHorizontalSplitBox::getSplitterRect(const Geometry& geo) const {
		float scaledLeftWidth = m_leftWidth * m_renderScale;
		float scaledThickness = m_splitterThickness * m_renderScale;
		float x = geo.position.x + scaledLeftWidth;
		return Rect(x, x + scaledThickness, geo.position.y, geo.position.y + geo.size.y);
	}

}
