using System.Collections.ObjectModel;
using System.Diagnostics;

namespace MooncastleEditor.Utilities
{
    public interface IUndoRedo
    {
        string Name { get; }
        void Undo();
        void Redo();
    }

    public class UndoRedoAction : IUndoRedo
    {
        private Action _undoAction;
        private Action _redoAction;

        public string Name { get; }
        public void Undo() => _undoAction();
        public void Redo() => _redoAction();

        public UndoRedoAction(string name)
        {
            Name = name;
        }

        public UndoRedoAction(Action undo, Action redo, string name)
        {
            Name = name;

            Debug.Assert(undo != null && redo != null);

            _undoAction = undo;
            _redoAction = redo;
        }

        public UndoRedoAction(string property, object instance, object undoValue, object redoValue, string name) :
            this
            (
                () => instance.GetType().GetProperty(property).SetValue(instance, undoValue),
                () => instance.GetType().GetProperty(property).SetValue(instance, redoValue), 
                name
            )
        {}
    }

    public class UndoRedo
    {
        private bool _enableAdd = true;

        private readonly ObservableCollection<IUndoRedo> _redoList = [];
        private readonly ObservableCollection<IUndoRedo> _undoList = [];
        public ReadOnlyObservableCollection<IUndoRedo> RedoList { get; }
        public ReadOnlyObservableCollection<IUndoRedo> UndoList { get; }

        public void Undo()
        {
            if (_undoList.Any())
            {
                var item = _undoList.Last();

                _undoList.RemoveAt(_undoList.Count - 1);
                _enableAdd = false;
                item.Undo();
                _enableAdd = true;

                _redoList.Insert(0, item);
            }
        }

        public void Redo()
        {
            if (_redoList.Any())
            {
                var item = _redoList.First();

                _redoList.RemoveAt(0);
                _enableAdd = false;
                item.Redo();
                _enableAdd = true;

                _undoList.Add(item);
            }
        }

        public void Add(IUndoRedo item)
        {
            if (_enableAdd)
            {
                _undoList.Add(item);
                _redoList.Clear();
            }
        }

        public void Reset()
        {
            _redoList.Clear();
            _undoList.Clear();
        }

        public UndoRedo()
        {
            RedoList = new ReadOnlyObservableCollection<IUndoRedo>(_redoList);
            UndoList = new ReadOnlyObservableCollection<IUndoRedo>(_undoList);
        }
    }
}
