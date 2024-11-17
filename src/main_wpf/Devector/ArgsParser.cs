using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace Devector
{
	public class ArgsParser
    {
        private static string m_help = "";
        private Dictionary<string, string> m_args = new Dictionary<string, string>();
        bool m_requirementSatisfied = true;

        public ArgsParser(string[] args, string description)
		{
            AddDescriptionToHelp(description);

            for (int i = 0; i < args.Length; i++)
			{
                // wait for the first param name
                if (args[i][0] == '-')
                {
                    string paramName = args[i].Substring(1);
                    string value = "";
                    if (i + 1 < args.Length && args[i + 1][0] != '-')
                    {
                        i++;
                        value = args[i];
                    }

                    m_args[paramName] = value;
                }
            }
        }

        public enum ArgType : uint
        {
            INT = 0,
            DOUBLE,
            STRING,
            END
        };

		private static readonly string[] ArgTypeStr = 
        {
            "int",
            "double",
            "string"
        };

        public void AddParamToHelp(string _arg, ArgType _type, bool _required, object _default, string _help)
        {
            string defaultStr = "";
            if (_type == ArgType.DOUBLE) defaultStr = Convert.ToString((double)_default);
            if (_type == ArgType.INT) defaultStr = Convert.ToString((int)_default);
            if (_type == ArgType.STRING) defaultStr = (string)_default;

            m_help += string.Format("-{0} \n\ttype: {1}, required: {2}, default: {3}\n{4}\n\n",
                _arg,
                ArgTypeStr[(int)_type],
                _required ? "true" : "false",
                _required ? "no default" : defaultStr,
                ("\t" + _help));
        }
        private void AddDescriptionToHelp(string _description) 
        {
            m_help += "Help:\n";
            m_help += $"Description: {_description}\n";
            m_help += "format: -paramName <value> or -h, -help to show this guide.\n";
            m_help += "Parameters:\n";
        }

        private void RequirementMsg(string _arg)
        {
            Console.WriteLine($"Required parameter \"{_arg}\" or its value was not provided.");
            m_requirementSatisfied = false;
        }

        public string GetString(string _arg, string _help, bool _required, string _defaultV = "")
        {
            AddParamToHelp(_arg, ArgType.STRING, _required, _defaultV, _help);

            if (!m_args.TryGetValue(_arg, out string v) || string.IsNullOrEmpty(v))
            {
                if (_required) RequirementMsg(_arg);
                return _defaultV;
            }

            return v;
        }

        public double GetDouble(string _arg, string _help, bool _required, double _defaultV)
        {
            AddParamToHelp(_arg, ArgType.DOUBLE, _required, _defaultV, _help);

            if (!m_args.TryGetValue(_arg, out string v) || string.IsNullOrEmpty(v))
            {
                if (_required) RequirementMsg(_arg);
                return _defaultV;
            }

            return double.Parse(v);
        }

        public int GetInt(string _arg, string _help, bool _required, int _defaultV)
        {
            AddParamToHelp(_arg, ArgType.INT, _required, _defaultV, _help);

            if (!m_args.TryGetValue(_arg, out string v) || string.IsNullOrEmpty(v))
            {
                if (_required) RequirementMsg(_arg);
                return _defaultV;
            }

            return int.Parse(v);
        }
        private void PrintHelp()
        {
            if (!m_args.ContainsKey("help") && 
                !m_args.ContainsKey("h") && 
                m_args.Count > 0 && 
                m_requirementSatisfied) return;

            Console.WriteLine($"\n{m_help}");
        }

        public bool IsRequirementSatisfied()
        {
            PrintHelp();
            return m_requirementSatisfied;
        }
    }
}
