#pragma once

#include "SWidget.h"
#include "Renderer.h"

namespace Silica {

	class SButton : public SWidget {
	public:

		struct Args {
			Vec2 padding = { 10.0f, 10.0f };
			uint32_t color = 0xFF444444;
			uint32_t hoverColor = 0xFF666666;
			uint32_t pressedColor = 0xFF222222;
			std::function<EventReply()> onClick = nullptr;
			WidgetPtr child = nullptr;
		};

		void construct(const Args& args);

		void computeDesiredSize() override;
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allotedGeometry) const override;

		EventReply onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseButtonUp(const Geometry& allotedGeometry, const Vec2& mousePos) override;

	private:

		Vec2 m_padding;
		uint32_t m_color;
		uint32_t m_hoverColor;
		uint32_t m_pressedColor;
		std::function<EventReply()> m_onClick;
		WidgetPtr m_child;

		bool m_isPressed = false;

		void addRectToDrawList(DrawList& drawList, const Geometry& geo, uint32_t color) const;

	};

}
