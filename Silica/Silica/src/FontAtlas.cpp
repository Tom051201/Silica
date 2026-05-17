#include "FontAtlas.h"

#include <fstream>

#include "vendor/stb_truetype.h"

namespace Silica {

	bool FontAtlas::loadFromFile(const char* filePath, float fontSize) {
		// -- Read Raw File Into Memory --
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return false;

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> ttfBuffer(size);
		if (!file.read((char*)ttfBuffer.data(), size)) return false;

		// -- Allocate Memory (512x512 atlas) --
		m_width = 512;
		m_height = 512;
		m_pixels.resize(m_width * m_height);

		stbtt_bakedchar cdata[96];

		// -- Bake The Font! --
		int result = stbtt_BakeFontBitmap(
			ttfBuffer.data(), 0, fontSize,
			m_pixels.data(), m_width, m_height,
			32, 96, cdata
		);

		if (result <= 0) return false;

		m_pixels[0] = 255;

		// -- Translate stb's Data Into Silica::Glyph Structs --
		for (int i = 0; i < 96; i++) {
			Glyph& g = m_glyphs[i];
			const stbtt_bakedchar& b = cdata[i];

			g.uvMin.x = (float)b.x0 / m_width;
			g.uvMin.y = (float)b.y0 / m_height;
			g.uvMax.x = (float)b.x1 / m_width;
			g.uvMax.y = (float)b.y1 / m_height;

			g.size.x = (float)(b.x1 - b.x0);
			g.size.y = (float)(b.y1 - b.y0);

			g.offset.x = b.xoff;
			g.offset.y = b.yoff;
			g.advanceX = b.xadvance;
		}

		return true;
	}

	const Glyph& FontAtlas::getGlyph(char c) const {
		int index = c - 32;
		if (index < 0 || index >= 96) {
			index = 0;
		}
		return m_glyphs[index];
	}

}
