using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Media;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static Devector.HardwareStats;
using System.Runtime.CompilerServices;
using static Devector.RamMappingViewModel;
using System.Collections.ObjectModel;

namespace Devector
{
	public class HardwareStatsViewModel : INotifyPropertyChanged
	{
		// Regs
		private string _af;
		private string _bc;
		private string _de;
		private string _hl;
		private string _sp;
		private string _pc;

		public string AF
		{
			get { return _af; }
			set
			{
				if (_af != value)
				{
					_af = value;
					OnPropertyChanged(nameof(AF));
				}
			}
		}

		public string BC
		{
			get { return _bc; }
			set
			{
				if (_bc != value)
				{
					_bc = value;
					OnPropertyChanged(nameof(BC));
				}
			}
		}

		public string DE
		{
			get { return _de; }
			set
			{
				if (_de != value)
				{
					_de = value;
					OnPropertyChanged(nameof(DE));
				}
			}
		}

		public string HL
		{
			get { return _hl; }
			set
			{
				if (_hl != value)
				{
					_hl = value;
					OnPropertyChanged(nameof(HL));
				}
			}
		}

		public string SP
		{
			get { return _sp; }
			set
			{
				if (_sp != value)
				{
					_sp = value;
					OnPropertyChanged(nameof(SP));
				}
			}
		}

		public string PC
		{
			get { return _pc; }
			set
			{
				if (_pc != value)
				{
					_pc = value;
					OnPropertyChanged(nameof(PC));
				}
			}
		}

		// Flags
		private string _c;
		private string _p;
		private string _ac;
		private string _z;
		private string _s;

		public string C
		{
			get { return _c; }
			set
			{
				if (_c != value)
				{
					_c = value;
					OnPropertyChanged(nameof(C));
				}
			}
		}

		public string P
		{
			get { return _p; }
			set
			{
				if (_p != value)
				{
					_p = value;
					OnPropertyChanged(nameof(P));
				}
			}
		}

		public string AC
		{
			get { return _ac; }
			set
			{
				if (_ac != value)
				{
					_ac = value;
					OnPropertyChanged(nameof(AC));
				}
			}
		}

		public string Z
		{
			get { return _z; }
			set
			{
				if (_z != value)
				{
					_z = value;
					OnPropertyChanged(nameof(Z));
				}
			}
		}

		public string S
		{
			get { return _s; }
			set
			{
				if (_s != value)
				{
					_s = value;
					OnPropertyChanged(nameof(S));
				}
			}
		}

		// Stack
		private string _spn10;
		private string _spn8;
		private string _spn6;
		private string _spn4;
		private string _spn2;
		private string _sp0;
		private string _sp2;
		private string _sp4;
		private string _sp6;
		private string _sp8;
		private string _sp10;

		public string SPN10
		{
			get { return _spn10; }
			set
			{
				if (_spn10 != value)
				{
					_spn10 = value;
					OnPropertyChanged(nameof(SPN10));
				}
			}
		}

		public string SPN8
		{
			get { return _spn8; }
			set
			{
				if (_spn8 != value)
				{
					_spn8 = value;
					OnPropertyChanged(nameof(SPN8));
				}
			}
		}

		public string SPN6
		{
			get { return _spn6; }
			set
			{
				if (_spn6 != value)
				{
					_spn6 = value;
					OnPropertyChanged(nameof(SPN6));
				}
			}
		}

		public string SPN4
		{
			get { return _spn4; }
			set
			{
				if (_spn4 != value)
				{
					_spn4 = value;
					OnPropertyChanged(nameof(SPN4));
				}
			}
		}

		public string SPN2
		{
			get { return _spn2; }
			set
			{
				if (_spn2 != value)
				{
					_spn2 = value;
					OnPropertyChanged(nameof(SPN2));
				}
			}
		}

		public string SP0
		{
			get { return _sp0; }
			set
			{
				if (_sp0 != value)
				{
					_sp0 = value;
					OnPropertyChanged(nameof(SP0));
				}
			}
		}

		public string SP2
		{
			get { return _sp2; }
			set
			{
				if (_sp2 != value)
				{
					_sp2 = value;
					OnPropertyChanged(nameof(SP2));
				}
			}
		}

		public string SP4
		{
			get { return _sp4; }
			set
			{
				if (_sp4 != value)
				{
					_sp4 = value;
					OnPropertyChanged(nameof(SP4));
				}
			}
		}

		public string SP6
		{
			get { return _sp6; }
			set
			{
				if (_sp6 != value)
				{
					_sp6 = value;
					OnPropertyChanged(nameof(SP6));
				}
			}
		}

		public string SP8
		{
			get { return _sp8; }
			set
			{
				if (_sp8 != value)
				{
					_sp8 = value;
					OnPropertyChanged(nameof(SP8));
				}
			}
		}

		public string SP10
		{
			get { return _sp10; }
			set
			{
				if (_sp10 != value)
				{
					_sp10 = value;
					OnPropertyChanged(nameof(SP10));
				}
			}
		}

		// Hardware
		private string _upTime;
		private string _cpuCicles;
		private string _lastRun;
		private string _crtXY;
		private string _frameCC;
		private string _frameNum;
		private string _displayMode;
		private string _scrollV;
		private string _rusLat;
		private string _inte;
		private string _iff;
		private string _hlta;

		public string UpTime
		{
			get { return _upTime; }
			set
			{
				if (_upTime != value)
				{
					_upTime = value;
					OnPropertyChanged(nameof(UpTime));
				}
			}
		}

		public string CpuCicles
		{
			get { return _cpuCicles; }
			set
			{
				if (_cpuCicles != value)
				{
					_cpuCicles = value;
					OnPropertyChanged(nameof(CpuCicles));
				}
			}
		}

		public string LastRun
		{
			get { return _lastRun; }
			set
			{
				if (_lastRun != value)
				{
					_lastRun = value;
					OnPropertyChanged(nameof(LastRun));
				}
			}
		}

		public string CrtXY
		{
			get { return _crtXY; }
			set
			{
				if (_crtXY != value)
				{
					_crtXY = value;
					OnPropertyChanged(nameof(CrtXY));
				}
			}
		}

		public string FrameCC
		{
			get { return _frameCC; }
			set
			{
				if (_frameCC != value)
				{
					_frameCC = value;
					OnPropertyChanged(nameof(FrameCC));
				}
			}
		}

		public string FrameNum
		{
			get { return _frameNum; }
			set
			{
				if (_frameNum != value)
				{
					_frameNum = value;
					OnPropertyChanged(nameof(FrameNum));
				}
			}
		}

		public string DisplayMode
		{
			get { return _displayMode; }
			set
			{
				if (_displayMode != value)
				{
					_displayMode = value;
					OnPropertyChanged(nameof(DisplayMode));
				}
			}
		}

		public string ScrollV
		{
			get { return _scrollV; }
			set
			{
				if (_scrollV != value)
				{
					_scrollV = value;
					OnPropertyChanged(nameof(ScrollV));
				}
			}
		}

		public string RusLat
		{
			get { return _rusLat; }
			set
			{
				if (_rusLat != value)
				{
					_rusLat = value;
					OnPropertyChanged(nameof(RusLat));
				}
			}
		}

		public string Inte
		{
			get { return _inte; }
			set
			{
				if (_inte != value)
				{
					_inte = value;
					OnPropertyChanged(nameof(Inte));
				}
			}
		}

		public string Iff
		{
			get { return _iff; }
			set
			{
				if (_iff != value)
				{
					_iff = value;
					OnPropertyChanged(nameof(Iff));
				}
			}
		}

		public string Hlta
		{
			get { return _hlta; }
			set
			{
				if (_hlta != value)
				{
					_hlta = value;
					OnPropertyChanged(nameof(Hlta));
				}
			}
		}

		// palette
		private SolidColorBrush _pal0;
		public SolidColorBrush Pal0
		{
			get => _pal0;
			set
			{
				_pal0 = value;
				OnPropertyChanged(nameof(Pal0));
			}
		}

		private SolidColorBrush _pal1;
		public SolidColorBrush Pal1
		{
			get => _pal1;
			set
			{
				_pal1 = value;
				OnPropertyChanged(nameof(Pal1));
			}
		}

		private SolidColorBrush _pal2;
		public SolidColorBrush Pal2
		{
			get => _pal2;
			set
			{
				_pal2 = value;
				OnPropertyChanged(nameof(Pal2));
			}
		}

		private SolidColorBrush _pal3;
		public SolidColorBrush Pal3
		{
			get => _pal3;
			set
			{
				_pal3 = value;
				OnPropertyChanged(nameof(Pal3));
			}
		}

		private SolidColorBrush _pal4;
		public SolidColorBrush Pal4
		{
			get => _pal4;
			set
			{
				_pal4 = value;
				OnPropertyChanged(nameof(Pal4));
			}
		}

		private SolidColorBrush _pal5;
		public SolidColorBrush Pal5
		{
			get => _pal5;
			set
			{
				_pal5 = value;
				OnPropertyChanged(nameof(Pal5));
			}
		}

		private SolidColorBrush _pal6;
		public SolidColorBrush Pal6
		{
			get => _pal6;
			set
			{
				_pal6 = value;
				OnPropertyChanged(nameof(Pal6));
			}
		}

		private SolidColorBrush _pal7;
		public SolidColorBrush Pal7
		{
			get => _pal7;
			set
			{
				_pal7 = value;
				OnPropertyChanged(nameof(Pal7));
			}
		}

		private SolidColorBrush _pal8;
		public SolidColorBrush Pal8
		{
			get => _pal8;
			set
			{
				_pal8 = value;
				OnPropertyChanged(nameof(Pal8));
			}
		}

		private SolidColorBrush _pal9;
		public SolidColorBrush Pal9
		{
			get => _pal9;
			set
			{
				_pal9 = value;
				OnPropertyChanged(nameof(Pal9));
			}
		}

		private SolidColorBrush _pal10;
		public SolidColorBrush Pal10
		{
			get => _pal10;
			set
			{
				_pal10 = value;
				OnPropertyChanged(nameof(Pal10));
			}
		}

		private SolidColorBrush _pal11;
		public SolidColorBrush Pal11
		{
			get => _pal11;
			set
			{
				_pal11 = value;
				OnPropertyChanged(nameof(Pal11));
			}
		}

		private SolidColorBrush _pal12;
		public SolidColorBrush Pal12
		{
			get => _pal12;
			set
			{
				_pal12 = value;
				OnPropertyChanged(nameof(Pal12));
			}
		}

		private SolidColorBrush _pal13;
		public SolidColorBrush Pal13
		{
			get => _pal13;
			set
			{
				_pal13 = value;
				OnPropertyChanged(nameof(Pal13));
			}
		}

		private SolidColorBrush _pal14;
		public SolidColorBrush Pal14
		{
			get => _pal14;
			set
			{
				_pal14 = value;
				OnPropertyChanged(nameof(Pal14));
			}
		}

		private SolidColorBrush _pal15;
		public SolidColorBrush Pal15
		{
			get => _pal15;
			set
			{
				_pal15 = value;
				OnPropertyChanged(nameof(Pal15));
			}
		}


		public struct Palette
		{
			int _idx;
			SolidColorBrush _color;

            public SolidColorBrush Color
			{
				get => _color;
			}
			public int Idx
			{
				get => _idx;
			}
        }

        private ObservableCollection<Palette> _palette;
        public ObservableCollection<Palette> Palette
        {
            get => _palette;
            set
            {
                _palette = value;
                OnPropertyChanged();
            }
        }

        // peripherial

        // ram-disks
        public RamMappingViewModel RamMappingViewModel { get; }

        public HardwareStatsViewModel()
        {
            RamMappingViewModel = new RamMappingViewModel();
        }

        public event PropertyChangedEventHandler? PropertyChanged;
		protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
		{
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}
	}
}
