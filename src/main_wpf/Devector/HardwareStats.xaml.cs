using dev;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
using static Devector.RamMappingViewModel;

namespace Devector
{
	public partial class HardwareStats : DevectorWindow
	{
		readonly DateTime startTime = DateTime.Now;
        private long _cc;

        private HardwareStatsViewModel ViewModel;
        // timer
        private DispatcherTimer? _halDisplayUpdateTmer;

		public HardwareStats()
		{
			InitializeComponent();

			ViewModel = new HardwareStatsViewModel();

            DataContext = ViewModel;
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
            ViewModel.AF = String.Format($"{af:X4}");

            var val = regsJ?.RootElement.GetProperty("bc").GetInt32() ?? 0;
            ViewModel.BC = String.Format($"{val:X4}");

            val = regsJ?.RootElement.GetProperty("de").GetInt32() ?? 0;
            ViewModel.DE = String.Format($"{val:X4}");

            val = regsJ?.RootElement.GetProperty("hl").GetInt32() ?? 0;
            ViewModel.HL = String.Format($"{val:X4}");

            var sp = regsJ?.RootElement.GetProperty("sp").GetInt32() ?? 0;
            ViewModel.SP = String.Format($"{sp:X4}");

            val = regsJ?.RootElement.GetProperty("pc").GetInt32() ?? 0;
            ViewModel.PC = String.Format($"{val:X4}");

            // Flags
            var flags = GetFlags(af);
            ViewModel.C = flags.c.ToString();
            ViewModel.P = flags.p.ToString();
            ViewModel.AC = flags.ac.ToString();
            ViewModel.Z = flags.z.ToString();
            ViewModel.S = flags.s.ToString();

            // Stack
            var data = new JsonObject
            {
                ["addr"] = sp
            };

            var jsonDoc = Hal?.Request(HAL.Req.GET_STACK_SAMPLE, data.ToJsonString());
            
            val = jsonDoc?.RootElement.GetProperty("-10").GetInt32() ?? 0;
            ViewModel.SPN10 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-8").GetInt32() ?? 0;
            ViewModel.SPN8 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-6").GetInt32() ?? 0;
            ViewModel.SPN6 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-4").GetInt32() ?? 0;
            ViewModel.SPN4 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("-2").GetInt32() ?? 0;
            ViewModel.SPN2 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("0").GetInt32() ?? 0;
            ViewModel.SP0 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("2").GetInt32() ?? 0;
            ViewModel.SP2 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("4").GetInt32() ?? 0;
            ViewModel.SP4 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("6").GetInt32() ?? 0;
            ViewModel.SP6 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("8").GetInt32() ?? 0;
            ViewModel.SP8 = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("10").GetInt32() ?? 0;
            ViewModel.SP10 = String.Format($"{val:X4}");


            // Hardware
            jsonDoc = Hal?.Request(HAL.Req.GET_HW_MAIN_STATS, "");
            var cc = jsonDoc?.RootElement.GetProperty("cc").GetInt64() ?? 0;
            ViewModel.CpuCicles = cc.ToString();
            ViewModel.LastRun = String.Format("{0}", cc - _cc);
            ViewModel.CrtXY = String.Format("{0}/{1}", 
                jsonDoc?.RootElement.GetProperty("rasterPixel").ToString() ?? "",
                jsonDoc?.RootElement.GetProperty("rasterLine").ToString() ?? "");
            ViewModel.FrameCC = jsonDoc?.RootElement.GetProperty("frameCc").ToString() ?? "";
            ViewModel.FrameNum = jsonDoc?.RootElement.GetProperty("frameNum").ToString() ?? "";
            ViewModel.DisplayMode = jsonDoc?.RootElement.GetProperty("displayMode").GetBoolean() ?? false ? "512" : "256";
            ViewModel.ScrollV = jsonDoc?.RootElement.GetProperty("scrollVert").ToString() ?? "";
            ViewModel.RusLat = jsonDoc?.RootElement.GetProperty("rusLat").GetBoolean() ?? false ? "(*)" : "( )";
            ViewModel.Inte = jsonDoc?.RootElement.GetProperty("inte").GetBoolean() ?? false ? "Off" : "On";
            ViewModel.Iff = jsonDoc?.RootElement.GetProperty("iff").GetBoolean() ?? false ? "Off" : "On";
            ViewModel.Hlta = jsonDoc?.RootElement.GetProperty("hlta").GetBoolean() ?? false ? "Off" : "On";
            _cc = cc;
            
            // palette
            ViewModel.Pal0 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette0").GetInt64() ?? 0);
            ViewModel.Pal1 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette1").GetInt64() ?? 0);
            ViewModel.Pal2 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette2").GetInt64() ?? 0);
            ViewModel.Pal3 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette3").GetInt64() ?? 0);
            ViewModel.Pal4 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette4").GetInt64() ?? 0);
            ViewModel.Pal5 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette5").GetInt64() ?? 0);
            ViewModel.Pal6 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette6").GetInt64() ?? 0);
            ViewModel.Pal7 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette7").GetInt64() ?? 0);
            ViewModel.Pal8 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette8").GetInt64() ?? 0);
            ViewModel.Pal9 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette9").GetInt64() ?? 0);
            ViewModel.Pal10 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette10").GetInt64() ?? 0);
            ViewModel.Pal11 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette11").GetInt64() ?? 0);
            ViewModel.Pal12 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette12").GetInt64() ?? 0);
            ViewModel.Pal13 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette13").GetInt64() ?? 0);
            ViewModel.Pal14 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette14").GetInt64() ?? 0);
            ViewModel.Pal15 = ColorFromBytes(jsonDoc?.RootElement.GetProperty("palette15").GetInt64() ?? 0);

            ObservableCollection<MappingData> palette = new ObservableCollection<Palette>();

            //for (int idx = 0; idx < (int)HAL.RAM_DISK_MAX; idx++)
            //{
            //    mappings.Add(new RamMappingViewModel.MappingData(idx, jsonDoc?.RootElement.GetProperty($"mapping{idx}").GetInt32() ?? 0));
            //}
            //ViewModel.RamMappingViewModel.MappingDataList = mappings;


            // RAM Disk
            jsonDoc = Hal?.Request(HAL.Req.GET_MEMORY_MAPPINGS, "");
            var enabledRamdiskIdx = jsonDoc?.RootElement.GetProperty("ramdiskIdx").GetInt32();

            ObservableCollection <MappingData> mappings = new ObservableCollection<MappingData>();

            for (int idx = 0; idx < (int)HAL.RAM_DISK_MAX; idx++)
            {
                mappings.Add( new RamMappingViewModel.MappingData(idx, jsonDoc?.RootElement.GetProperty($"mapping{idx}").GetInt32() ?? 0));
            }
            ViewModel.RamMappingViewModel.MappingDataList = mappings;


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
			ViewModel.UpTime = (DateTime.Now - startTime).ToString(@"hh\:mm\:ss");
        }

	}
}
