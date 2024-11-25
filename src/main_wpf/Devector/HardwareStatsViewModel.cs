using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
