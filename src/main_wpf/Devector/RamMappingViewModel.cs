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
			for (ulong i = 0; i < HAL.RAM_DISK_MAX; i++)
			{
				MappingDataList.Add(new MappingData(0)); // Initialize with zero
			}
		}

		// Specific method for getting Page Ram
		public string GetPageRam(int index) =>
			index >= 0 && index < MappingDataList.Count
				? MappingDataList[index].pageRam.ToString()
				: "N/A";

		// Method for getting Page Stack
		public string GetPageStack(int index) =>
			index >= 0 && index < MappingDataList.Count
				? MappingDataList[index].pageStack.ToString()
				: "N/A";

		// Method for getting Mode Ram
		public string GetModeRam(int index) =>
			index >= 0 && index < MappingDataList.Count
				? MappingDataList[index].ModeRamToString()
				: "N/A";

		// Method for getting Mode Stack
		public string GetModeStack(int index) =>
			index >= 0 && index < MappingDataList.Count
				? MappingDataList[index].ModeStackToString()
				: "N/A";

		// INotifyPropertyChanged Implementation
		public event PropertyChangedEventHandler PropertyChanged;
		protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
		{
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}
	}
}