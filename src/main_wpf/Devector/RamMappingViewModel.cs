using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Media;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static Devector.HardwareStats;
using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;
using dev;

namespace Devector
{
	public class RamMappingViewModel : INotifyPropertyChanged
	{
        public struct MappingData
        {
            public int idx;
            public int pageRam;    // Ram-Disk 64k page idx accesssed via non-stack instructions (all instructions except mentioned below)
            public int pageStack;  // Ram-Disk 64k page idx accesssed via the stack instructions (Push, Pop, XTHL, Call, Ret, C*, R*, RST)
            public bool modeStack; // enabling stack mapping
            public bool modeRamA;  // enabling ram [0xA000-0xDFFF] mapped into the the Ram-Disk
            public bool modeRam8;  // enabling ram [0x8000-0x9FFF] mapped into the the Ram-Disk
            public bool modeRamE;  // enabling ram [0xE000-0xFFFF] mapped into the the Ram-Disk   

            public MappingData(int ramDiskIdx, int data = 0)
            {
                idx = ramDiskIdx;
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
                return modeStack ? "On" : "Off";
            }

            public string MappingRam { get => ModeRamToString() + " / " + pageRam.ToString(); }
            public string MappingStack { get => ModeStackToString() + " / " + pageStack.ToString(); }

            public int Idx { get => idx; }
        }

        private ObservableCollection<MappingData> _mappingDataList;
		public ObservableCollection<MappingData> MappingDataList
		{
			get => _mappingDataList;
			set
			{
				_mappingDataList = value;
                OnPropertyChanged();
            }
		}

		public RamMappingViewModel()
		{
            // Initialize with sample data
            MappingDataList = new ObservableCollection<MappingData>();
			for (int i = 0; i < (int)HAL.RAM_DISK_MAX; i++)
			{
				MappingDataList.Add(new MappingData(i));
			}
        }

        // INotifyPropertyChanged Implementation
        public event PropertyChangedEventHandler PropertyChanged;
		protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
		{
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}
	}
}