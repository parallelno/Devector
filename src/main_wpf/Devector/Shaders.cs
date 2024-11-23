namespace Devector
{
    public static class Shaders
    {
		public const string vtxShaderDisplay = @"
			#version 330 core
			precision highp float;

			layout (location = 0) in vec3 pos;
			layout (location = 1) in vec2 uv;
			
			uniform vec4 m_uvMinMax;

			out vec2 uv0;

			void main()
			{
				// remap 0-1 to m_uvMinMax
				uv0 = mix(m_uvMinMax.xy, m_uvMinMax.zw, uv);

				gl_Position = vec4(pos.xyz, 1.0f);
			}
		";
        public const string fragShaderDisplay = @"
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
		";
    }
}
