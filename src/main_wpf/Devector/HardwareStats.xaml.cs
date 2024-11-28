using dev;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace Devector
{
	public partial class HardwareStats : DevectorWindow
	{
		readonly DateTime startTime = DateTime.Now;
        private long _cc;

        private HardwareStatsViewModel _viewModel;
		// timer
		private DispatcherTimer? _halDisplayUpdateTmer;

		public HardwareStats()
		{
			InitializeComponent();

			_viewModel = new HardwareStatsViewModel();
			DataContext = _viewModel;
            UpdateData();
            UpdateDataByTimer();

            // init timer
            _halDisplayUpdateTmer = new DispatcherTimer(TimeSpan.FromSeconds(1),
										 DispatcherPriority.Render,
										 Update,
										 Dispatcher.CurrentDispatcher);
			_halDisplayUpdateTmer.Start();
		}

		private void Update(object? sender, EventArgs e)
		{
            UpdateDataByTimer();
		}

        private record Flags ( int c, int p, int ac, int z, int s );
        private Flags GetFlags(int af)
        {
            return new Flags(
                (af & 0x01),
                (af & 0x04) >> 2,
                (af & 0x10) >> 4,
                (af & 0x40) >> 6,
                (af & 0x80) >> 7
            );
        }

        private void UpdateData()
		{
            // Regs
            var regsJ = Hal?.Request(HAL.Req.GET_REGS, "");
            var af = regsJ?.RootElement.GetProperty("af").GetInt32() ?? 0;
            _viewModel.AF = String.Format($"{af:X4}");

            var val = regsJ?.RootElement.GetProperty("bc").GetInt32() ?? 0;
            _viewModel.BC = String.Format($"{val:X4}");

            val = regsJ?.RootElement.GetProperty("de").GetInt32() ?? 0;
            _viewModel.DE = String.Format($"{val:X4}");

            val = regsJ?.RootElement.GetProperty("hl").GetInt32() ?? 0;
            _viewModel.HL = String.Format($"{val:X4}");

            var sp = regsJ?.RootElement.GetProperty("sp").GetInt32() ?? 0;
            _viewModel.SP = String.Format($"{sp:X4}");

            val = regsJ?.RootElement.GetProperty("pc").GetInt32() ?? 0;
            _viewModel.PC = String.Format($"{val:X4}");

            // Flags
            var flags = GetFlags(af);
            _viewModel.C = flags.c.ToString();
            _viewModel.P = flags.p.ToString();
            _viewModel.AC = flags.ac.ToString();
            _viewModel.Z = flags.z.ToString();
            _viewModel.S = flags.s.ToString();

            // Stack
            var data = new JsonObject
            {
                ["addr"] = sp
            };

            var jsonDoc = Hal?.Request(HAL.Req.GET_STACK_SAMPLE, data.ToJsonString());
            
            val = jsonDoc?.RootElement.GetProperty("-10").GetInt32() ?? 0;
            _viewModel.SPN10 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-8").GetInt32() ?? 0;
            _viewModel.SPN8 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-6").GetInt32() ?? 0;
            _viewModel.SPN6 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-4").GetInt32() ?? 0;
            _viewModel.SPN4 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-2").GetInt32() ?? 0;
            _viewModel.SPN2 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("0").GetInt32() ?? 0;
            _viewModel.SP0 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("2").GetInt32() ?? 0;
            _viewModel.SP2 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("4").GetInt32() ?? 0;
            _viewModel.SP4 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("6").GetInt32() ?? 0;
            _viewModel.SP6 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("8").GetInt32() ?? 0;
            _viewModel.SP8 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("10").GetInt32() ?? 0;
            _viewModel.SP10 = String.Format($"{val:X4}");


            // Hardware
            jsonDoc = Hal?.Request(HAL.Req.GET_HW_MAIN_STATS, "");
            var cc = jsonDoc?.RootElement.GetProperty("cc").GetInt64() ?? 0;
            _viewModel.CpuCicles = cc.ToString();
            _viewModel.LastRun = String.Format("{0}", cc - _cc);
            _viewModel.CrtXY = String.Format("{0}/{1}", 
                jsonDoc?.RootElement.GetProperty("rasterPixel").ToString() ?? "",
                jsonDoc?.RootElement.GetProperty("rasterLine").ToString() ?? "");
            _viewModel.FrameCC = jsonDoc?.RootElement.GetProperty("frameCc").ToString() ?? "";
            _viewModel.FrameNum = jsonDoc?.RootElement.GetProperty("frameNum").ToString() ?? "";
            _viewModel.DisplayMode = jsonDoc?.RootElement.GetProperty("displayMode").GetBoolean() ?? false ? "512" : "256";
            _viewModel.ScrollV = jsonDoc?.RootElement.GetProperty("scrollVert").ToString() ?? "";
            _viewModel.RusLat = jsonDoc?.RootElement.GetProperty("rusLat").GetBoolean() ?? false ? "(*)" : "( )";
            _viewModel.Inte = jsonDoc?.RootElement.GetProperty("inte").GetBoolean() ?? false ? "Off" : "On";
            _viewModel.Iff = jsonDoc?.RootElement.GetProperty("iff").GetBoolean() ?? false ? "Off" : "On";
            _viewModel.Hlta = jsonDoc?.RootElement.GetProperty("hlta").GetBoolean() ?? false ? "Off" : "On";
            _cc = cc;
            
            // palette
            _viewModel.Pal0 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette0").GetInt64() ?? 0);
            _viewModel.Pal1 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette1").GetInt64() ?? 0);
            _viewModel.Pal2 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette2").GetInt64() ?? 0);
            _viewModel.Pal3 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette3").GetInt64() ?? 0);
            _viewModel.Pal4 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette4").GetInt64() ?? 0);
            _viewModel.Pal5 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette5").GetInt64() ?? 0);
            _viewModel.Pal6 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette6").GetInt64() ?? 0);
            _viewModel.Pal7 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette7").GetInt64() ?? 0);
            _viewModel.Pal8 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette8").GetInt64() ?? 0);
            _viewModel.Pal9 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette9").GetInt64() ?? 0);
            _viewModel.Pal10 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette10").GetInt64() ?? 0);
            _viewModel.Pal11 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette11").GetInt64() ?? 0);
            _viewModel.Pal12 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette12").GetInt64() ?? 0);
            _viewModel.Pal13 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette13").GetInt64() ?? 0);
            _viewModel.Pal14 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette14").GetInt64() ?? 0);
            _viewModel.Pal15 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette15").GetInt64() ?? 0);


            // Ram-Disk
            jsonDoc = Hal?.Request(HAL.Req.GET_MEMORY_MAPPINGS, "");
            var ramdiskIdx = jsonDoc?.RootElement.GetProperty("ramdiskIdx").GetInt32();

            // FDC

        }

        private SolidColorBrush ColorFromBytes(long colorValue)
        {
            return new SolidColorBrush(Color.FromArgb(
                (byte)((colorValue >> 24) & 0xFF),  // Alpha
                (byte)((colorValue >> 16) & 0xFF),  // Red
                (byte)((colorValue >> 8) & 0xFF),   // Green
                (byte)(colorValue & 0xFF)           // Blue
            ));
        }

        private void UpdateDataByTimer()
		{
			_viewModel.UpTime = (DateTime.Now - startTime).ToString(@"hh\:mm\:ss");
        }

	}
}
