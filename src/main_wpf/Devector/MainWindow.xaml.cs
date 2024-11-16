using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using dev;
using static Devector.Consts;
using static Devector.Shaders;

using static System.Net.Mime.MediaTypeNames;
using System.Text.Json;
using System.Numerics;
using System;

namespace Devector
{

	public partial class MainWindow : Window
	{
		[DllImport("kernel32.dll")]
		private static extern bool AllocConsole();

		public MainWindow()
		{
			InitializeComponent();

			// opens a console window
			AllocConsole();

			Loaded += MainWindow_Loaded;
			Closed += MainWindow_Closed;
		}

        //////////////////////////////////
        ///
        /// 
        /// 
        public const int HW_FRAME_W = 768;                 // Vector06c frame including borders
        public const int HW_FRAME_H = 312;                 // Vector06c frame including borders

        bool m_glInited = false;
		HAL? Hal;

		ulong m_ccLast = 0;
		int m_rasterPixel = 0;
		int m_rasterLine = 0;
		bool m_displayIsHovered = false;
		Vector4 m_scrollV_crtXY_highlightMul;

		int m_vramShaderId = INVALID_ID;
		int m_vramTexId = INVALID_ID;
		int m_vramMatId = INVALID_ID;

		private void MainWindow_Loaded(object sender, RoutedEventArgs e)
		{
			var viewportW = viewport.ActualWidth;
			var viewportH = viewport.ActualHeight;

			Hal = ((Devector.App)System.Windows.Application.Current).Hal;

			m_glInited = Hal?.CreateGfxContext(viewport.Handle, (int)viewportW, (int)viewportH) ?? false;

			if (!m_glInited) return;

			m_glInited |= DisplayWindowInit((int)viewportW, (int)viewportH);
		}

		private void MainWindow_Closed(object? sender, EventArgs e)
		{

		}

		protected override void OnRender(DrawingContext drawingContext)
		{
			base.OnRender(drawingContext);
			// This will only be called when WPF determines a redraw is needed
			//RedrawMyContent();
		}

		private void Button_Click(object sender, RoutedEventArgs e)
		{
			RedrawMyContent();
		}


		void RedrawMyContent()
		{

			if (!m_glInited) return;

			var cc = Hal?.ReqCC();
			label.Content = "Counter: " + cc.ToString();

			var isRunning = Hal?.ReqIsRunning() ?? false;


			var viewportW = viewport.ActualWidth;
			var viewportH = viewport.ActualHeight;

			UpdateData(isRunning, (int)viewportW, (int)viewportH);
			DrawDisplay(isRunning, (int)viewportW, (int)viewportH);
		}

		private bool DisplayWindowInit(int _viewportW, int _viewportH)
		{
			m_vramShaderId = Hal?.InitShader(viewport.Handle, vtxShaderDisplay, fragShaderDisplay) ?? INVALID_ID;
			if (m_vramShaderId == INVALID_ID) return false;

			m_vramTexId = Hal?.InitTexture(viewport.Handle, HW_FRAME_W, HW_FRAME_H) ?? INVALID_ID;
			if (m_vramTexId == INVALID_ID) return false;


			var jsonDoc = Hal?.Request(HAL.Req.GET_DISPLAY_BORDER_LEFT, "");
			var elem = new JsonElement();
			var res = jsonDoc?.RootElement.TryGetProperty("borderLeft", out elem);
			if (res == false) return false;
			int borderLeft = elem.GetInt32();


			var textureIds = new[] { m_vramTexId };
			var shaderParamNames = new[] { "m_activeArea_pxlSize", "m_scrollV_crtXY_highlightMul", "m_bordsLRTB" };
			var shaderParamValues = new[] {
				new Vector4(HAL.ACTIVE_AREA_W, HAL.ACTIVE_AREA_H, HAL.FRAME_PXL_SIZE_W, HAL.FRAME_PXL_SIZE_H ),
				new Vector4(255.0f * HAL.FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f),
				new Vector4(
							borderLeft * HAL.FRAME_PXL_SIZE_W,
							(borderLeft + HAL.ACTIVE_AREA_W) * HAL.FRAME_PXL_SIZE_W,
							HAL.SCAN_ACTIVE_AREA_TOP * HAL.FRAME_PXL_SIZE_H,
							(HAL.SCAN_ACTIVE_AREA_TOP + HAL.ACTIVE_AREA_H) * HAL.FRAME_PXL_SIZE_H
					)
			};

			m_vramMatId = Hal?.InitMaterial(viewport.Handle, m_vramShaderId, textureIds,
					shaderParamNames, shaderParamValues, _viewportW, _viewportH) ?? INVALID_ID;

			return m_vramMatId != INVALID_ID;
		}

		private bool UpdateData(bool _isRunning, int _viewportW, int _viewportH)
		{
			if (!m_glInited) return false;

			var cc = Hal?.ReqCC();
			var ccDiff = cc - m_ccLast;
			m_ccLast = cc ?? 0;

			m_scrollV_crtXY_highlightMul.W = 1.0f;

			if (!_isRunning)
			{
				if (ccDiff > 0)
				{
					var jsonDoc = Hal?.Request(HAL.Req.GET_DISPLAY_DATA, "");
					var elem = new JsonElement();

					var res = jsonDoc?.RootElement.TryGetProperty("rasterPixel", out elem);
					if (res == false) return false;
					m_rasterPixel = elem.GetInt32();
					res = jsonDoc?.RootElement.TryGetProperty("rasterLine", out elem);
					if (res == false) return false;
					m_rasterLine = elem.GetInt32();

				}
				if (!m_displayIsHovered)
				{
					m_scrollV_crtXY_highlightMul.Y = m_rasterPixel * HAL.FRAME_PXL_SIZE_W;
					m_scrollV_crtXY_highlightMul.Z = m_rasterLine * HAL.FRAME_PXL_SIZE_H;
					m_scrollV_crtXY_highlightMul.W = HAL.SCANLINE_HIGHLIGHT_MUL;
				}
			}

			var jsonDoc2 = Hal?.Request(HAL.Req.GET_SCROLL_VERT, "");
			var elem2 = new JsonElement();
			var res2 = jsonDoc2?.RootElement.TryGetProperty("scrollVert", out elem2);
			if (res2 == false) return false;
			m_scrollV_crtXY_highlightMul.X = elem2.GetInt32();

			// TODO: send m_scrollV_crtXY_highlightMul to a shader

			Hal?.UpdateFrameTexture(viewport.Handle, m_vramTexId, _isRunning);

			return true;
		}

		private bool DrawDisplay(bool _isRunning, int _viewportW, int _viewportH)
		{
            if (!m_glInited) return false;

            /*
                    float border = 0;
                    ImVec2 borderMin;
                    ImVec2 borderMax;
                    switch (m_borderType)
                    {
                    case dev::DisplayWindow::BorderType::FULL:
                        borderMin = { 0.0f, 0.0f };
                        borderMax = { 1.0f, 1.0f };
                        break;

                    case dev::DisplayWindow::BorderType::NORMAL:
                        border = Display::BORDER_VISIBLE;
                        [[fallthrough]];

                    case dev::DisplayWindow::BorderType::NONE:
                    {
                        int borderLeft = m_hardwareP->Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");

                        borderMin = {
                            (borderLeft - border * 2) * FRAME_PXL_SIZE_W,
                            (Display::SCAN_ACTIVE_AREA_TOP - border) * FRAME_PXL_SIZE_H };
                        borderMax = {
                            borderMin.x + (Display::ACTIVE_AREA_W + border * 4) * FRAME_PXL_SIZE_W,
                            borderMin.y + (Display::ACTIVE_AREA_H + border * 2) * FRAME_PXL_SIZE_H };
                        break;
                    }
                    }

                    ImVec2 displaySize;
                    switch (m_displaySize)
                    {
                    case dev::DisplayWindow::DisplaySize::R256_256:
                        displaySize.x = 256.0f;
                        displaySize.y = 256.0f;
                        break;
                    case dev::DisplayWindow::DisplaySize::R512_256:
                        displaySize.x = 512.0f;
                        displaySize.y = 256.0f;
                        break;
                    case dev::DisplayWindow::DisplaySize::R512_512:
                        displaySize.x = 512.0f;
                        displaySize.y = 512.0f;
                        break;
                    case dev::DisplayWindow::DisplaySize::MAX:
                    {
                        ImGuiStyle& style = ImGui::GetStyle();
                        auto wMax = ImGui::GetWindowWidth() - style.FramePadding.x * 4;
                        auto hMax = ImGui::GetWindowHeight() - style.FramePadding.y * 14;

                        displaySize.x = wMax;
                        displaySize.y = displaySize.x * WINDOW_ASPECT;
                        if (displaySize.y > hMax)
                        {
                            displaySize.y = hMax;
                            displaySize.x = displaySize.y / WINDOW_ASPECT;
                        }
                        break;
                    }
                    }

                    auto framebufferTex = m_glUtils.GetFramebufferTexture(m_vramMatId);
                    ImGui::Image((void*)(intptr_t)framebufferTex, displaySize, borderMin, borderMax);
                    //m_displayIsHovered = ImGui::IsItemHovered();
                    */

            var res = Hal?.Draw(viewport.Handle, m_vramMatId, _viewportW, _viewportH);
			if (res != (int)ErrCode.NO_ERRORS) return false;

            /*if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                ImGui::OpenPopup(m_contextMenuName);
            }*/

            return true;
        }
    }
}