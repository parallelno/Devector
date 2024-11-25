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
            var jsonDoc = Hal?.Request(HAL.Req.GET_REGS, "");
            var af = jsonDoc?.RootElement.GetProperty("af").GetInt32() ?? 0;
            _viewModel.AF = String.Format($"{af:X4}");

            var val = jsonDoc?.RootElement.GetProperty("bc").GetInt32() ?? 0;
            _viewModel.BC = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("de").GetInt32() ?? 0;
            _viewModel.DE = String.Format($"{val:X4}");

            val = jsonDoc?.RootElement.GetProperty("hl").GetInt32() ?? 0;
            _viewModel.HL = String.Format($"{val:X4}");

            var sp = jsonDoc?.RootElement.GetProperty("sp").GetInt32() ?? 0;
            _viewModel.SP = String.Format($"{sp:X4}");

            val = jsonDoc?.RootElement.GetProperty("pc").GetInt32() ?? 0;
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

            jsonDoc = Hal?.Request(HAL.Req.GET_STACK_SAMPLE, data.ToJsonString());
            
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
            jsonDoc = Hal?.Request(HAL.Req.GET_CC, "");
            var cc = jsonDoc?.RootElement.GetProperty("cc").GetInt64();
            _viewModel.CpuCicles = cc.ToString() ?? "";

            // Ram-Disk

            // FDC

        }

        private void UpdateDataByTimer()
		{
			_viewModel.UpTime = (DateTime.Now - startTime).ToString(@"hh\:mm\:ss");
        }

	}
}
