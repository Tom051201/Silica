#pragma once

#include <string>

#include "SWidget.h"
#include "FontAtlas.h"

namespace Silica {

	class SEditableText : public SWidget {
	public:

		struct Args {
			std::string hintText = "Type here...";
			Color textColor = Color::white();
			Color backgroundColor = Color(30, 30, 30);
			Color focusedColor = Color(50, 50, 50);
			FontAtlas* font = nullptr;
		};

		void construct(const Args& args);

		void computeDesiredSize() override;
		void arrangeChildren(const Geometry& allocatedGeometry) override;
		void onDraw(DrawList& outDrawList, const Geometry& allocatedGeometry) const override;

		EventReply onMouseButtonDown(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onMouseMove(const Geometry& allotedGeometry, const Vec2& mousePos) override;
		EventReply onChar(char c) override;
		EventReply onKeyDown(int key) override;

	private:

		std::string m_text;
		std::string m_hintText;
		Color m_textColor;
		Color m_backgroundColor;
		Color m_focusedColor;
		FontAtlas* m_font;

		void addRectToDrawList(DrawList& drawList, const Geometry& geo, Color color) const;

	};

}
