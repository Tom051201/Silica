#include "SSeparator.h"

#include "Theme.h"
#include "Renderer.h"

namespace Silica {

	void SSeparator::construct(const Args& args) {
		m_orientation = args.orientation;
		m_thickness = args.thickness;
		m_color = args.color.value_or(GetTheme().Border_Primary);
	}

	void SSeparator::computeDesiredSize() {
		if (m_orientation == Orientation::Horizontal) {
			m_desiredSize = Vec2(0.0f, m_thickness);
		}
		else {
			m_desiredSize = Vec2(m_thickness, 0.0f);
		}
	}

	void SSeparator::arrangeChildren(const Geometry& allocatedGeometry) {
		SWidget::arrangeChildren(allocatedGeometry);
	}

	void SSeparator::onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const {
		if (m_color.a() > 0) {
			outDrawList.addRect(allocatedGeometry, m_color);
		}
	}

}
