using System.Windows.Input;

namespace MooncastleEditor
{
    class RelayCommand<T> : ICommand
    {
        private readonly Action<T> _execute;
        private readonly Predicate<T> _canExecute;

        public RelayCommand(Action<T> execute, Predicate<T> canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public event EventHandler CanExecuteChanged
        {
            add => CommandManager.RequerySuggested += value;
            remove => CommandManager.RequerySuggested -= value;
        }

        public bool CanExecute(object parameter)
        {
            if (_canExecute == null) return true;

            if (TryGetParameter(parameter, out var value))
                return _canExecute(value);

            return false;
        }

        public void Execute(object parameter)
        {
            if (TryGetParameter(parameter, out var value))
                _execute(value);
        }

        private static bool TryGetParameter(object parameter, out T value)
        {
            if (parameter == null)
            {
                value = default;
                return true;
            }

            if (parameter is T t)
            {
                value = t;
                return true;
            }

            try
            {
                value = (T)Convert.ChangeType(parameter, typeof(T));
                return true;
            }
            catch
            {
                value = default;
                return false;
            }
        }
    }
}