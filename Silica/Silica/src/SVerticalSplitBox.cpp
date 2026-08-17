#include "SVerticalSplitBox.h"

#include "Theme.h"
#include <algorithm>

namespace Silica {

	void SVerticalSplitBox::construct(const Args& args) {
		m_topHeight = args.topHeight;
		m_splitterThickness = args.splitterThickness;
		m_top = args.topContent;
		m_bottom = args.bottomContent;
	}

	Rect SVerticalSplitBox::getSplitterRect(const Geometry& geo) const {
		float scaledTopHeight = m_topHeight * m_renderScale;
		float scaledThickness = m_splitterThickness * m_renderScale;
		float y = geo.position.y + scaledTopHeight;
		return Rect(geo.position.x, geo.position.x + geo.size.x, y, y + scaledThickness);
	}

	void SVerticalSplitBox::computeDesiredSize() {
		m_desiredSize = { 0, 0 };
		float scaledTopHeight = m_topHeight * m_renderScale;
		float scaledThickness = m_splitterThickness * m_renderScale;

		if (m_top) {
			m_top->computeDesiredSize();
			m_desiredSize.y += scaledTopHeight;
			m_desiredSize.x = std::max(m_desiredSize.x, m_top->getDesiredSize().x);
		}

		m_desiredSize.y += scaledThickness;

		if (m_bottom) {
			m_bottom->computeDesiredSize();
			m_desiredSize.y += m_bottom->getDesiredSize().y;
			m_desiredSize.x = std::max(m_desiredSize.x, m_bottom->getDesiredSize().x);
		}
	}

	void SVerticalSplitBox::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);

		float scaledTopHeight = m_topHeight * m_renderScale;
		float scaledThickness = m_splitterThickness * m_renderScale;

		if (m_top) {
			m_top->arrangeChildren({ allocatedGeometry.position, {allocatedGeometry.size.x, scaledTopHeight} });
		}

		if (m_bottom) {
			float bottomY = allocatedGeometry.position.y + scaledTopHeight + scaledThickness;
			float bottomHeight = std::max(0.0f, allocatedGeometry.size.y - scaledTopHeight - scaledThickness);

			m_bottom->arrangeChildren({
				{ allocatedGeometry.position.x, bottomY },
				{ allocatedGeometry.size.x, bottomHeight }
				});
		}
	}

	void SVerticalSplitBox::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_top) m_top->onDraw(outDrawList, m_top->getAllocatedGeometry());
		if (m_bottom) m_bottom->onDraw(outDrawList, m_bottom->getAllocatedGeometry());

		Rect sRect = getSplitterRect(allocatedGeometry);
		Geometry sGeo = { {sRect.left, sRect.top}, {sRect.right - sRect.left, sRect.bottom - sRect.top} };

		Color barColor = GetTheme().Surface_Tertiary;
		if (m_isDraggingSplitter) barColor = GetTheme().Accent_Primary;
		else if (m_isHoveredSplitter) barColor = GetTheme().Element_Hover;

		outDrawList.addRect(sGeo, barColor);
	}

	void SVerticalSplitBox::setRenderScale(float scale) {
		m_renderScale = scale;
		if (m_top) m_top->setRenderScale(scale);
		if (m_bottom) m_bottom->setRenderScale(scale);
	}

	EventReply SVerticalSplitBox::onMouseMove(const Geometry& geo, const Vec2& pos) {
		m_isHoveredSplitter = getSplitterRect(geo).contains(pos);

		if (m_isDraggingSplitter) {
			float deltaY = (pos.y - m_dragStartY) / m_renderScale;
			m_topHeight = std::max(50.0f, m_initialTopHeight + deltaY);

			float maxTopHeight = (geo.size.y / m_renderScale) - 50.0f;
			if (maxTopHeight > 50.0f) m_topHeight = std::min(m_topHeight, maxTopHeight);

			arrangeChildren(geo);
			return EventReply::handled();
		}

		if (m_isHoveredSplitter) {
			Platform::setCursor(Platform::Cursor::ResizeNS);
			return EventReply::handled();
		}

		if (m_top && m_top->onMouseMove(m_top->getAllocatedGeometry(), pos).isHandled) return EventReply::handled();
		if (m_bottom && m_bottom->onMouseMove(m_bottom->getAllocatedGeometry(), pos).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SVerticalSplitBox::onMouseButtonDown(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		if (btn == MouseButton::Left && getSplitterRect(geo).contains(pos)) {
			m_isDraggingSplitter = true;
			m_dragStartY = pos.y;
			m_initialTopHeight = m_topHeight;
			SWidget::setCapturedWidget(this);
			return EventReply::handled();
		}

		if (m_top && m_top->onMouseButtonDown(m_top->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();
		if (m_bottom && m_bottom->onMouseButtonDown(m_bottom->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SVerticalSplitBox::onMouseButtonUp(const Geometry& geo, const Vec2& pos, MouseButton btn) {
		if (btn == MouseButton::Left && m_isDraggingSplitter) {
			m_isDraggingSplitter = false;
			SWidget::setCapturedWidget(nullptr);
			return EventReply::handled();
		}

		if (m_top && m_top->onMouseButtonUp(m_top->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();
		if (m_bottom && m_bottom->onMouseButtonUp(m_bottom->getAllocatedGeometry(), pos, btn).isHandled) return EventReply::handled();

		return EventReply::unhandled();
	}

	EventReply SVerticalSplitBox::onMouseWheel(const Geometry& geo, const Vec2& pos, float delta) {
		if (m_top && m_top->onMouseWheel(m_top->getAllocatedGeometry(), pos, delta).isHandled) return EventReply::handled();
		if (m_bottom && m_bottom->onMouseWheel(m_bottom->getAllocatedGeometry(), pos, delta).isHandled) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SVerticalSplitBox::onDragOver(const Geometry& geo, const Vec2& pos, const DragDropPayload& payload) {
		if (m_top && m_top->onDragOver(m_top->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		if (m_bottom && m_bottom->onDragOver(m_bottom->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		return EventReply::unhandled();
	}

	EventReply SVerticalSplitBox::onDrop(const Geometry& geo, const Vec2& pos, const DragDropPayload& payload) {
		if (m_top && m_top->onDrop(m_top->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		if (m_bottom && m_bottom->onDrop(m_bottom->getAllocatedGeometry(), pos, payload).isHandled) return EventReply::handled();
		return EventReply::unhandled();
	}

}
