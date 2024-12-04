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
            _viewModel.Mapping1 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping0").GetInt32() ?? 0);
            var mappingData1 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping1").GetInt32() ?? 0);
            var mappingData2 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping2").GetInt32() ?? 0);
            var mappingData3 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping3").GetInt32() ?? 0);
            var mappingData4 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping4").GetInt32() ?? 0);
            var mappingData5 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping5").GetInt32() ?? 0);
            var mappingData6 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping6").GetInt32() ?? 0);
            var mappingData7 = new MappingData(jsonDoc?.RootElement.GetProperty("mapping7").GetInt32() ?? 0);

            for (ulong ramDiskIdx = 0; ramDiskIdx < HAL.RAM_DISK_MAX; ramDiskIdx++)
            {
                var mappingData = new MappingData(jsonDoc?.RootElement.GetProperty($"mapping{ramDiskIdx}").GetInt32() ?? 0);
            }
            // mapping ram mode
            //_viewModel.MappingRamMode1 = mappingData0.ModeRamToString();
            //_viewModel.MappingRamMode2 = mappingData1.ModeRamToString();
            //_viewModel.MappingRamMode3 = mappingData2.ModeRamToString();
            //_viewModel.MappingRamMode4 = mappingData3.ModeRamToString();
            //_viewModel.MappingRamMode5 = mappingData4.ModeRamToString();
            //_viewModel.MappingRamMode6 = mappingData5.ModeRamToString();
            //_viewModel.MappingRamMode7 = mappingData6.ModeRamToString();
            //_viewModel.MappingRamMode8 = mappingData7.ModeRamToString();
            //// mapping ram page
            //_viewModel.MappingRamPage1 = mappingData0.ToString();
            //_viewModel.MappingRamPage2 = mappingData1.ToString();
            //_viewModel.MappingRamPage3 = mappingData2.ToString();
            //_viewModel.MappingRamPage4 = mappingData3.ToString();
            //_viewModel.MappingRamPage5 = mappingData4.ToString();
            //_viewModel.MappingRamPage6 = mappingData5.ToString();
            //_viewModel.MappingRamPage7 = mappingData6.ToString();
            //_viewModel.MappingRamPage8 = mappingData7.ToString();
            //// mapping stack mode
            //_viewModel.MappingStackMode1 = mappingData0.ModeStackToString();
            //_viewModel.MappingStackMode2 = mappingData1.ModeStackToString();
            //_viewModel.MappingStackMode3 = mappingData2.ModeStackToString();
            //_viewModel.MappingStackMode4 = mappingData3.ModeStackToString();
            //_viewModel.MappingStackMode5 = mappingData4.ModeStackToString();
            //_viewModel.MappingStackMode6 = mappingData5.ModeStackToString();
            //_viewModel.MappingStackMode7 = mappingData6.ModeStackToString();
            //_viewModel.MappingStackMode8 = mappingData7.ModeStackToString();
            //// mapping stack page
            //_viewModel.MappingStackPage1 = mappingData0.ToString();
            //_viewModel.MappingStackPage2 = mappingData1.ToString();
            //_viewModel.MappingStackPage3 = mappingData2.ToString();
            //_viewModel.MappingStackPage4 = mappingData3.ToString();
            //_viewModel.MappingStackPage5 = mappingData4.ToString();
            //_viewModel.MappingStackPage6 = mappingData5.ToString();
            //_viewModel.MappingStackPage7 = mappingData6.ToString();
            //_viewModel.MappingStackPage8 = mappingData7.ToString();


            // FDC

        }

        public struct MappingData
        {
            public int pageRam;    // Ram-Disk 64k page idx accesssed via non-stack instructions (all instructions except mentioned below)
            public int pageStack;  // Ram-Disk 64k page idx accesssed via the stack instructions (Push, Pop, XTHL, Call, Ret, C*, R*, RST)
            public bool modeStack; // enabling stack mapping
            public bool modeRamA;  // enabling ram [0xA000-0xDFFF] mapped into the the Ram-Disk
            public bool modeRam8;  // enabling ram [0x8000-0x9FFF] mapped into the the Ram-Disk
            public bool modeRamE;  // enabling ram [0xE000-0xFFFF] mapped into the the Ram-Disk   

            public MappingData(int data = 0)
            {
                pageRam = data & 0x2;
                pageStack = (data >> 2) & 0x2;
                modeStack = ((data >> 4) & 0x1) != 0;
                modeRamA = ((data >> 5) & 0x1) != 0;
                modeRam8 = ((data >> 6) & 0x1) != 0;
                modeRamE = ((data >> 7) & 0x1) != 0;
            }
            public string ModeRamToString()
            {
                string mapping = modeRam8 ? "8" : "-";
                mapping += modeRam8 ? "AC" : "-";
                mapping += modeRam8 ? "E" : "-";
                return mapping;
            }
            public string ModeStackToString()
            {
                return modeStack? "On" : "Off";
            }
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
