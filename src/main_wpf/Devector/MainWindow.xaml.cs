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

		//////////////////////////////////
		///
		/// 
		/// 

		bool m_glInited = false;
		HAL? Hal;

		int m_vramShaderId = INVALID_ID;
		int m_vramTexId = INVALID_ID;
		int m_vramMatId = INVALID_ID;

		private int counter = 0;

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

		private bool DisplayWindowInit(int _viewportW, int _viewportH)
		{
			m_vramShaderId = Hal?.InitShader(viewport.Handle, vtxShaderDisplay, fragShaderDisplay) ?? INVALID_ID;
			if (m_vramShaderId == INVALID_ID) return false;

			m_vramTexId = Hal?.InitTexture(viewport.Handle, _viewportW, _viewportH) ?? INVALID_ID;
			if (m_vramTexId == INVALID_ID) return false;


			//int borderLeft = Hal?.Request(HAL.Req.GET_DISPLAY_BORDER_LEFT)->at("borderLeft");
			var jsonS = Hal?.Request(HAL.Req.GET_DISPLAY_BORDER_LEFT, "") ?? "";//->at("borderLeft");
																				// Parse the JSON string to a JsonDocument
			using (JsonDocument jsonDoc = JsonDocument.Parse(jsonS))
			{
                // Check if the key exists before accessing it
                if (jsonDoc.RootElement.TryGetProperty("borderLeft", out JsonElement borderLeftElement))
                {
                    // Convert the JsonElement to an integer
                    int borderLeft = borderLeftElement.GetInt32();
                    Console.WriteLine("Border Left: " + borderLeft);
                }
            }

			/*
            GLUtils::ShaderParams shaderParams = {
					{ "m_activeArea_pxlSize", { Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H }},
					{ "m_scrollV_crtXY_highlightMul", { 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f } },
					{ "m_bordsLRTB", {
							borderLeft * FRAME_PXL_SIZE_W,
							(borderLeft + Display::ACTIVE_AREA_W) * FRAME_PXL_SIZE_W,
							static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
							static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H)* FRAME_PXL_SIZE_H  } }
			};

				m_vramMatId = Hal?.InitMaterial(viewport.Handle, m_vramShaderId, { m_vramTexId }, shaderParams,
					Display::FRAME_W, Display::FRAME_H, false);

			if (m_vramMatId == INVALID_ID) return false;
			*/
			return true;
		}

		void RedrawMyContent()
		{

			if (!m_glInited) return;

			counter++;
			var cc = Hal?.GetCC();
			label.Content = "Counter: " + cc.ToString();


			var viewportW = viewport.ActualWidth;
			var viewportH = viewport.ActualHeight;

			//Hal?.UpdateData(true, (int)viewportW, (int)viewportH);
			//Hal?.DrawDisplay((int)viewportW, (int)viewportH);
		}

	}
}