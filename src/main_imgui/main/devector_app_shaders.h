const char* DISPLAY_VTX_SHADER_S =
R"#(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uv;

	out vec2 uv0;

	void main()
	{
		uv0 = vec2(uv.x, 1.0f - uv.y);
		gl_Position = vec4(pos.xyz, 1.0f);
	}
)#";

const char* DISPLAY_FRAG_SHADER_S =
R"#(
	#version 330 core
	precision highp float;
	precision highp int;

	in vec2 uv0;

	uniform sampler2D texture0;
	uniform vec4 m_activeArea_pxlSize;
	uniform vec4 m_bordsLRTB;
	uniform vec4 m_scrollV_crtXY_highlightMul;

	layout (location = 0) out vec4 out0;

	void main()
	{
		vec2 uv = uv0;
		float bordL = m_bordsLRTB.x;
		float bordR = m_bordsLRTB.y;
		float bordT = m_bordsLRTB.z;
		float bordB = m_bordsLRTB.w;
		float highlightMul = m_scrollV_crtXY_highlightMul.w;
		vec2 crt = m_scrollV_crtXY_highlightMul.yz;
		vec2 pxlSize = m_activeArea_pxlSize.zw;

		// vertical scrolling
		if (uv.x >= bordL &&
			uv.x < bordR &&
			uv.y >= bordT &&
			uv.y < bordB)
		{
			uv.y -= m_scrollV_crtXY_highlightMul.x;
			// wrap V
			uv.y += uv.y < bordT ? m_activeArea_pxlSize.y * pxlSize.y : 0.0f;
		}

		vec3 color = texture(texture0, uv).rgb;

		// crt scanline highlight
		if (highlightMul < 1.0f)
		{
			if (uv.y >= crt.y &&
				uv.y < crt.y + pxlSize.y &&
				uv.x < crt.x + pxlSize.x )
			{
				// highlight the rasterized pixels of the current crt line
				color.xyz = vec3(0.3f, 0.3f, 0.3f) + color.xyz * 2.0f;
			}
			else
			if ((uv.y >= crt.y &&
				uv.y < crt.y + pxlSize.y &&
				uv.x >= crt.x + pxlSize.x ) || uv.y > crt.y + pxlSize.y)
			{
				// renders not rasterized pixels yet
				color.xyz *= m_scrollV_crtXY_highlightMul.w;
			}
		}

		out0 = vec4(color, 1.0f);
	}
)#";


const char* MEM_DISPLAY_VTX_SHADER_S =
R"(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 vtxPos;
	layout (location = 1) in vec2 vtxUV;

	out vec2 uv0;

	void main()
	{
		uv0 = vtxUV;
		gl_Position = vec4(vtxPos.xyz, 1.0f);
	}
)";

const char* MEM_DISPLAY_FRAG_SHADER_S
= R"(
	#version 330 core
	precision highp float;
	precision highp int;

	in vec2 uv0;

	uniform sampler2D texture0; // global ram values
	uniform sampler2D texture1; // .xy - highlight reads, .zw - highlight writes
	uniform vec4 globalColorBg;
	uniform vec4 globalColorFg;
	uniform vec4 highlightRead;
	uniform vec4 highlightWrite;
	uniform vec4 highlightIdxMax;

	layout (location = 0) out vec4 out0;

	#define BYTE_COLOR_MULL 0.6
	#define BACK_COLOR_MULL 0.7

	int GetBit(float _val, int _bitIdx) {
		return (int(_val * 255.0) >> _bitIdx) & 1;
	}

	void main()
	{
		float isAddrBelow32K = 1.0 - step(0.5, uv0.y);
		vec2 uv = vec2( uv0.y * 2.0, uv0.x / 2.0 + isAddrBelow32K * 0.5);
		float byte = texture(texture0, uv).r;

		float isOdd8K = step(0.5, fract(uv0.x / 0.5));
		isOdd8K = mix(isOdd8K, 1.0 - isOdd8K, isAddrBelow32K);
		vec3 bgColor = mix(globalColorBg.xyz, globalColorBg.xyz * BACK_COLOR_MULL, isOdd8K);

		int bitIdx = 7 - int(uv0.x * 1024.0) & 7;
		int isBitOn = GetBit(byte, bitIdx);

		// highlight every second column
		int isByteOdd = (int(uv0.x * 512.0)>>2) & 1;
		vec3 byteColor = mix(globalColorFg.xyz * BYTE_COLOR_MULL, globalColorFg.xyz, float(isByteOdd));

		vec3 color = mix(bgColor, byteColor, float(isBitOn));

		// highlight
		vec4 rw = texture(texture1, uv);
		float reads = (rw[1] * 256.0f + rw[0]) * 256.0f / highlightIdxMax.x;
		float writes = (rw[3] * 255.0f + rw[2] ) * 256.0f / highlightIdxMax.x;
		vec3 readsColor = reads * highlightRead.rgb * highlightRead.a;
		vec3 writesColor = writes * highlightWrite.rgb * highlightWrite.a;
		vec3 rwColor = readsColor + writesColor;
		color = mix(color * 0.8f, color * 0.5f + rwColor * 0.5f + color * rwColor * 2.0f, rwColor);

		out0 = vec4(color, globalColorBg.a);
	}
)";