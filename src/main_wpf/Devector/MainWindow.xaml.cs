using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Text.Json;
using System.Numerics;

using dev;
using static Devector.Consts;
using static Devector.Shaders;
using static Devector.App;
using System;
using Microsoft.VisualBasic.FileIO;

namespace Devector
{
	public enum BorderSizeOption
	{
		None,
		Normal,
		Full
	}

    public enum ViewportSizeOption
    {
        R256_256,
        R512_512,
        MAX
    }

    public partial class MainWindow : Window
	{
		//////////////////////////////////
		//
		// consts
		// 
		//////////////////////////////////

		private const int HW_FRAME_W = 768;                 // Vector06c frame including borders
		private const int HW_FRAME_H = 312;                 // Vector06c frame including borders

		private const double DISPLAY_RATIO = 4.0 / 3.0;
		private const double VIEWPORT_MARGIN = 5.0;			// the margin between the window and the viewport

		private bool m_glInited = false;
		private HAL? Hal;

		private ulong m_ccLast = 0;
		private int m_rasterPixel = 0;
		private int m_rasterLine = 0;
		private bool m_displayIsHovered = false;
		private BorderSizeOption m_borderSizeOption = BorderSizeOption.Normal;
        private ViewportSizeOption m_viewportSizeOption = ViewportSizeOption.MAX;
        private Vector2 viewportSize;

        private string m_matParamName_scrollV_crtXY_highlightMul = "m_scrollV_crtXY_highlightMul";
		private string m_matParamName_bordsLRTB = "m_bordsLRTB";
		private string m_matParamName_uvMinMax = "m_uvMinMax";
		private Vector4 m_matParamVal_scrollV_crtXY_highlightMul;
		private Vector4 m_matParamVal_bordsLRTB;
		private Vector4 m_matParamVal_uvMinMax;
		private int m_matParamId_scrollV_crtXY_highlightMul = INVALID_ID;
		private int m_matParamId_bordsLRTB = INVALID_ID;
		private int m_matParamId_uvMinMax = INVALID_ID;

		private int m_vramShaderId = INVALID_ID;
		private int m_vramTexId = INVALID_ID;
		private int m_vramMatId = INVALID_ID;

        private volatile bool halIsRunning = false;
		private ulong halCc = 0;

		public MainWindow()
		{
			InitializeComponent();

			Hal = ((Devector.App)System.Windows.Application.Current).Hal;

			DataContext = this; // Set the DataContext to allow binding to commands

			Loaded += MainWindow_Loaded;
			//LocationChanged += MainWindow_LocationChanged;
			SizeChanged += MainWindow_SizeChanged;
			var app = (App)Application.Current;

			app.halDisplayUpdate += Update;			// if the display rendering is called only by the timer, it brings artefacts - white blinking during resize. calling rendering by resize event doesn't fix the issue.
			CompositionTarget.Rendering += DrawEvent;   // a partial solution is to render onRender
		}

		private void DrawEvent(object? sender, EventArgs e)
		{
			Draw();
		}

		private void MainWindow_Loaded(object sender, RoutedEventArgs e)
		{
			if (m_glInited) return;

			var viewportW = viewport.ActualWidth;
			var viewportH = viewport.ActualHeight;

			m_glInited = Hal?.CreateGfxContext(viewport.Handle, (int)viewportW, (int)viewportH) ?? false;

			if (!m_glInited) return;

			m_glInited |= DisplayInit((int)viewportW, (int)viewportH);
		}

		private void MainWindow_LocationChanged(object? sender, EventArgs e)
		{
			//Draw();
		}
		private void MainWindow_SizeChanged(object sender, SizeChangedEventArgs e)
		{
			viewportSize = GetViewportSize(m_viewportSizeOption);
            viewport.Width = viewportSize.X;
            viewport.Height = viewportSize.Y;
        }
		protected override void OnClosed(EventArgs e)
		{
			base.OnClosed(e);
		}

		private void Draw()
		{
			DrawDisplay(halIsRunning, (int)viewportSize.X, (int)viewportSize.Y);
		}

		private bool DisplayInit(int _viewportW, int _viewportH)
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

			m_matParamVal_scrollV_crtXY_highlightMul = new Vector4(255.0f * HAL.FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f);
			m_matParamVal_bordsLRTB = new Vector4(
							borderLeft * HAL.FRAME_PXL_SIZE_W,
							(borderLeft + HAL.ACTIVE_AREA_W) * HAL.FRAME_PXL_SIZE_W,
							HAL.SCAN_ACTIVE_AREA_TOP * HAL.FRAME_PXL_SIZE_H,
							(HAL.SCAN_ACTIVE_AREA_TOP + HAL.ACTIVE_AREA_H) * HAL.FRAME_PXL_SIZE_H);
			m_matParamVal_uvMinMax = GetDisplayUVMinMax(m_borderSizeOption);

			var textureIds = new[] { m_vramTexId };

			var shaderParamNames = new[] { 
				"m_activeArea_pxlSize", 
				m_matParamName_scrollV_crtXY_highlightMul, 
				m_matParamName_bordsLRTB,
				m_matParamName_uvMinMax,
			};
			var shaderParamValues = new[] {
				new Vector4(HAL.ACTIVE_AREA_W, HAL.ACTIVE_AREA_H, HAL.FRAME_PXL_SIZE_W, HAL.FRAME_PXL_SIZE_H ),
				m_matParamVal_scrollV_crtXY_highlightMul,
				m_matParamVal_bordsLRTB,
				m_matParamVal_uvMinMax
			};

			m_vramMatId = Hal?.InitMaterial(viewport.Handle, m_vramShaderId, textureIds,
					shaderParamNames, shaderParamValues, _viewportW, _viewportH) ?? INVALID_ID;
			if (m_vramMatId == INVALID_ID) return false;

			m_matParamId_scrollV_crtXY_highlightMul = Hal?.GetMaterialParamId(
				viewport.Handle, m_vramMatId, m_matParamName_scrollV_crtXY_highlightMul) ?? INVALID_ID;
			if (m_matParamId_scrollV_crtXY_highlightMul == INVALID_ID) return false;

			m_matParamId_bordsLRTB = Hal?.GetMaterialParamId(
				viewport.Handle, m_vramMatId, m_matParamName_bordsLRTB) ?? INVALID_ID;
			if (m_matParamId_bordsLRTB == INVALID_ID) return false;

			m_matParamId_uvMinMax = Hal?.GetMaterialParamId(
			viewport.Handle, m_vramMatId, m_matParamName_uvMinMax) ?? INVALID_ID;
			if (m_matParamId_uvMinMax == INVALID_ID) return false;

			return true;
		}

		private void Update(object? sender, EventArgs e)
		{
			if (!m_glInited) return;

			halCc = Hal?.ReqCC() ?? 0;
			halIsRunning = Hal?.ReqIsRunning() ?? false;

			var viewportW = viewport.ActualWidth;
			var viewportH = viewport.ActualHeight;
			UpdateData(halIsRunning, (int)viewportW, (int)viewportH);

			//Draw(); // disabled because it's rendered in onRender
		}

		private bool UpdateData(bool _isRunning, int _viewportW, int _viewportH)
		{
			if (!m_glInited) return false;

			var cc = Hal?.ReqCC();
			var ccDiff = cc - m_ccLast;
			m_ccLast = cc ?? 0;

			m_matParamVal_scrollV_crtXY_highlightMul.W = 1.0f;

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
					m_matParamVal_scrollV_crtXY_highlightMul.Y = m_rasterPixel * HAL.FRAME_PXL_SIZE_W;
					m_matParamVal_scrollV_crtXY_highlightMul.Z = m_rasterLine * HAL.FRAME_PXL_SIZE_H;
					m_matParamVal_scrollV_crtXY_highlightMul.W = HAL.SCANLINE_HIGHLIGHT_MUL;
				}
			}

			var jsonDoc2 = Hal?.Request(HAL.Req.GET_SCROLL_VERT, "");
			var elem2 = new JsonElement();
			var res2 = jsonDoc2?.RootElement.TryGetProperty("scrollVert", out elem2);
			if (res2 == false) return false;
			m_matParamVal_scrollV_crtXY_highlightMul.X = elem2.GetInt32();

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

		//////////////////////////////////
		//
		// Main Menu
		//
		//////////////////////////////////
		
		private void OpenMenuItem_Click(object sender, RoutedEventArgs e)
		{
			var app = (App)Application.Current;
			app.OpenFile();
		}

		private void RecentFilesMenuItem_Click(object sender, RoutedEventArgs e)
		{
			MessageBox.Show("New command executed.");
		}

		private void SaveRecMenuItem_Click(object sender, RoutedEventArgs e)
		{
			MessageBox.Show("Save command executed.");
		}

		private void ExitMenuItem_Click(object sender, RoutedEventArgs e)
		{
			Close();
		}

		private void ToolOpenMenuItem_Click(object sender, RoutedEventArgs e)
		{
			var windowName = ((MenuItem)sender).Header.ToString();

			switch (windowName)
			{
				case "HardwareStats":
					break;

				case "Disasm":
					break;

				case "Breakpoints":
					break;

				case "Watchpoints":
					break;
			}

			// create and show a window
		}

		private void AboutMenuItem_Click(object sender, RoutedEventArgs e)
		{
			MessageBox.Show("About dialog.");
		}

		private ICommand? _openRecentFileCommand;
		public ICommand OpenRecentFileCommand => _openRecentFileCommand ??= new RelayCommand<RecentFile>(OpenRecentFile);

		private void OpenRecentFile(RecentFile? file)
		{
			if (file is not null)
			{
				var app = (App)Application.Current;
				app.Load(file);
			}
		}

		//////////////////////////////////
		//
		// Context Menu
		//
		//////////////////////////////////

		private void Viewport_ContextMenuOpen(object sender, MouseButtonEventArgs e)
		{
			Point mousePos = e.GetPosition(viewport);             // Get the position of the mouse relative to the Viewport control
			Rect viewportBounds = new Rect(0, 0, viewport.ActualWidth, viewport.ActualHeight);

			if (viewportBounds.Contains(mousePos))
			{
				viewport.ContextMenu.IsOpen = true;
			}
		}

		public double[] DisplayAspects =
		{
			DISPLAY_RATIO,
			1.0,
		};

		private Vector4 GetDisplayUVMinMax(BorderSizeOption borderType)
		{
			float border = 0;
			var uvMinMax = new Vector4(0, 0, 1, 1);

			switch (borderType)
			{
				/*
				case BorderSizeOption.Full:
					uvMinMax = new(0, 0, 1, 1);
					break;
				*/
				case BorderSizeOption.Normal:
					border = HAL.BORDER_VISIBLE;
					goto case BorderSizeOption.None;

				case BorderSizeOption.None:
					var jsonDoc = Hal?.Request(HAL.Req.GET_DISPLAY_BORDER_LEFT, "");
					var elem = new JsonElement();
					var res = jsonDoc?.RootElement.TryGetProperty("borderLeft", out elem);
					if (res == false) return uvMinMax;
					int borderLeft = elem.GetInt32();

					uvMinMax.X = (borderLeft - border * 2) * HAL.FRAME_PXL_SIZE_W;
					uvMinMax.Y = (HAL.SCAN_ACTIVE_AREA_TOP - border) * HAL.FRAME_PXL_SIZE_H;

					uvMinMax.Z = uvMinMax.X +
						(HAL.ACTIVE_AREA_W + border * 4) * HAL.FRAME_PXL_SIZE_W;
					uvMinMax.W = uvMinMax.Y +
						(HAL.ACTIVE_AREA_H + border * 2) * HAL.FRAME_PXL_SIZE_H;
					break;
			}

			return uvMinMax;
		}

        private Vector2 GetViewportSize(ViewportSizeOption viewportSizeOption)
        {
			var size = new Vector2();

			switch(viewportSizeOption)
			{
				case ViewportSizeOption.R256_256:
                    size.X = size.Y = 256;
                    break;

                case ViewportSizeOption.R512_512:
                    size.X = size.Y = 512;
					break;

                case ViewportSizeOption.MAX:
                    var viewportW = this.ActualWidth - VIEWPORT_MARGIN;
                    var viewportH = this.ActualHeight - menuBar.ActualHeight - SystemParameters.WindowCaptionHeight - VIEWPORT_MARGIN * 2;

                    if (viewportW <= 0 || viewportH <= 0) return new Vector2(0,0);

                    if (viewportW / viewportH > DISPLAY_RATIO)
                    {
                        viewportW = viewportH * DISPLAY_RATIO;
                    }
                    else
                    {
                        viewportH = viewportW / DISPLAY_RATIO;
                    }
					size.X = (float)viewportW;
                    size.Y = (float)viewportH;
					break;
            }

			return size;
        }

        private void BorderSizeUpdate(object sender, RoutedEventArgs e)
		{
			if (sender is MenuItem menuItem && menuItem.Tag is BorderSizeOption option)
			{
				m_matParamVal_uvMinMax = GetDisplayUVMinMax(option);

				Hal?.UpdateMaterialParam(viewport.Handle, m_vramMatId,
					m_matParamId_uvMinMax, m_matParamVal_uvMinMax);
			}
		}

        private void DisplaySizeUpdate(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem menuItem && menuItem.Tag is ViewportSizeOption option)
            {
				m_viewportSizeOption = option;
                viewportSize = GetViewportSize(option);
                viewport.Width = viewportSize.X;
                viewport.Height = viewportSize.Y;
            }
        }
    }
}