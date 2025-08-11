using Microsoft.VisualStudio.OLE.Interop;
using MooncastleEditor.DllWrappers;
using MooncastleEditor.GameProject;
using MooncastleEditor.Utilities;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Runtime.Serialization;

namespace MooncastleEditor.Components
{
    [DataContract]
    [KnownType(typeof(Transform))]
    [KnownType(typeof(Script))]
    [KnownType(typeof(Geometry))]
    class GameEntity : ViewModelBase
    {
        private IdType _entityId = ID.invalidId;
        public IdType EntityId
        {
            get => _entityId;
            private set
            {
                if (_entityId != value)
                {
                    _entityId = value;
                    OnPropertyChanged(nameof(EntityId));
                }
            }
        }

        private bool _isActive;
        public bool IsActive
        {
            get => _isActive;
            set
            {
                if (_isActive != value)
                {
                    _isActive = value;

                    if (_isActive == true)
                    {
                        _components.ToList().ForEach(x => x.Load());
                        EntityId = EngineAPI.EntityAPI.CreateGameEntity(this);
                        Debug.Assert(ID.isValid(EntityId));
                    }
                    else if (ID.isValid(EntityId))
                    {
                        EngineAPI.EntityAPI.RemoveGameEntity(this);
                        _components.ToList().ForEach(x => x.Unload());
                        EntityId = ID.invalidId;
                    }

                    OnPropertyChanged(nameof(IsActive));
                }
            }
        }

        [DataMember]
        private bool _isEnabled = true;
        public bool IsEnabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        private string _name;
        [DataMember]
        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        [DataMember]
        public Scene ParentScene { get; private set; }

        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<Component> _components = [];
        public ReadOnlyObservableCollection<Component> Components { get; private set; }

        public Component GetComponent(Type type) => Components.FirstOrDefault(c => c.GetType() == type);
        public T GetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;

        public bool AddComponent(Component component)
        {
            Debug.Assert(component != null);

            if (!Components.Any(x => x.GetType() == component.GetType()))
            {
                //An inactive entity should NOT be set to active if a component is added.
                var wasActive = IsActive;

                IsActive = false;
                _components.Add(component);
                IsActive = wasActive;

                return true;
            }

            Logger.Log(MessageType.Warning, $"Entity {Name} already has a {component.GetType().Name} component.");

            return false;
        }

        public void RemoveComponent(Component component)
        {
            Debug.Assert(component != null);

            if (component is Transform) return;


            if (_components.Contains(component))
            {
                IsActive = false;
                _components.Remove(component);
                IsActive = true;
            }
        }

        [OnDeserialized]
        void OnDeserialized(StreamingContext context)
        {
            if (_components != null)
            {
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }
        }

        public GameEntity(Scene scene)
        {
            Debug.Assert(scene != null);
            ParentScene = scene;
            _components.Add(new Transform(this));
            OnDeserialized(new StreamingContext());
        }
    }

    abstract class MSEntity : ViewModelBase
    {
        //Enable to update entities that are selected.
        private bool _enableUpdates = true;

        private bool? _isEnabled;
        public bool? IsEnabled
        {
            get => _isEnabled;
            set
            {
                if (_isEnabled != value)
                {
                    _isEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        private string _name; 
        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        private readonly ObservableCollection<IMSComponent> _components = [];
        public ReadOnlyObservableCollection<IMSComponent> Components {get; }

        public T GetMSComponent<T>() where T : IMSComponent
        {
            return (T)Components.FirstOrDefault(x => x.GetType() == typeof(T));
        }

        public List<GameEntity> SelectedEntities { get; }

        protected virtual bool UpdateGameEntities(string propertyName)
        {
            switch (propertyName)
            {
                case (nameof(IsEnabled)): SelectedEntities.ForEach(x => x.IsEnabled = IsEnabled.Value); return true;
                case (nameof(Name)): SelectedEntities.ForEach(x => x.Name = Name); return true;
            }

            return false;
        }

        private void MakeComponentList()
        {
            _components.Clear();

            var firstEntity = SelectedEntities.FirstOrDefault();
            if (firstEntity == null)
            {
                return;
            }

            foreach (var component in firstEntity.Components)
            {
                var type = component.GetType();

                if (!SelectedEntities.Skip(1).Any(entity => entity.GetComponent(type) == null))
                {
                    Debug.Assert(Components.FirstOrDefault(x => x.GetType() == type) == null);
                    _components.Add(component.GetMultiSelectionComponent(this));
                }
            }
        }

        public static int? GetMixedValue<T>(List<T> objects, Func<T, int> getProperty)
        {
            var value = getProperty(objects.First());

            return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
        }


        public static float? GetMixedValue<T>(List<T> objects, Func<T, float> getProperty)
        {
            var value = getProperty(objects.First());

            return objects.Skip(1).Any(x => !getProperty(x).IsTheSameAs(value)) ? null : value;
        }

        public static bool? GetMixedValue<T>(List<T> objects, Func<T, bool> getProperty)
        {
            var value = getProperty(objects.First());

            return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
        }

        public static string GetMixedValue<T>(List<T> objects, Func<T, string> getProperty)
        {
            var value = getProperty(objects.First());

            return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
        }


        protected virtual bool UpdateMSGameEntity()
        {
            IsEnabled = GetMixedValue(SelectedEntities, new Func<GameEntity, bool>(x => x.IsEnabled));
            Name = GetMixedValue(SelectedEntities, new Func<GameEntity, string>(x => x.Name));

            return true;
        }

        public void Refresh()
        {
            _enableUpdates = false;

            UpdateMSGameEntity();
            MakeComponentList();

            _enableUpdates = true;
        }

        protected MSEntity(List<GameEntity> entities)
        {
            Debug.Assert(entities?.Any() == true);
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntities = entities;
            
            PropertyChanged += (s, e) => { if (_enableUpdates) UpdateGameEntities(e.PropertyName); };
        }
    }

    class MSGameEntity : MSEntity
    {
        public MSGameEntity(List<GameEntity> entities) : base (entities)
        {
            Refresh();
        }
    }
}
