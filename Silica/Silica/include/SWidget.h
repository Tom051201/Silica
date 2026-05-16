#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "Geometry.h"

namespace Silica {

	struct DrawList;

	class SWidget;
	using WidgetPtr = std::shared_ptr<SWidget>;

	class SWidget {
	public:

		virtual ~SWidget() = default;

		virtual void computeDesiredSize() = 0;
		virtual void arrangeChildren(const Geometry& allocatedGeometry) = 0;
		virtual void onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const = 0;

		virtual EventReply onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) { return EventReply::unhandled(); }
		virtual EventReply onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) { return EventReply::unhandled(); }
		virtual EventReply onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) { return EventReply::unhandled(); }

		Vec2 getDesiredSize() const { return m_desiredSize; }

	protected:

		Vec2 m_desiredSize = Vec2::zero();
		bool m_isHovered = false;

	};



	// ----- Make Widget Factory -----
	template<typename T>
	std::shared_ptr<T> MakeWidget(const typename T::Args& args) {
		std::shared_ptr<T> widget = std::make_shared<T>();
		widget->construct(args);
		return widget;
	}

}
