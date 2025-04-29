using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace MooncastleEditor.GameProject
{
    [DataContract]
    public class Scene : ViewModelBase
    {
        private string name;

        [DataMember]
        public string Name
        {
            get 
            {
                return name;
            }
            set
            {
                if (name != value)
                {
                    name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        [DataMember]
        public Project Project { get; private set; }

        public Scene(string name, Project project)
        {
            Debug.Assert(project != null);
            Project = project;
            name = name;
        }
    }
}
