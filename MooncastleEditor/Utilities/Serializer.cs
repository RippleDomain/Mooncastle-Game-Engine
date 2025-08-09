using System.Diagnostics;
using System.IO;
using System.Runtime.Serialization;

namespace MooncastleEditor.Utilities
{
    public static class Serializer
    {
        public static void ToFile<T>(T instance, string path)
        {
            try
            {
                using var fileStream = new FileStream(path, FileMode.Create);
                var serializer = new DataContractSerializer(typeof(T));
                serializer.WriteObject(fileStream, instance);
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to serialize {instance} to {path}");

                throw;
            }
        }

        internal static T FromFile<T>(string path)
        {
            try
            {
                using var fileStream = new FileStream(path, FileMode.Open);
                var serializer = new DataContractSerializer(typeof(T));
                T instance = (T)serializer.ReadObject(fileStream);

                return instance;
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to deserialize {path}");

                throw;
            }
        }
    }
}
