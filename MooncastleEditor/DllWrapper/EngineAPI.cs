using MooncastleEditor.Components;
using MooncastleEditor.EngineAPIStructs;
using MooncastleEditor.GameProject;
using MooncastleEditor.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace MooncastleEditor.EngineAPIStructs
{
    [StructLayout(LayoutKind.Sequential)]
    class TransformComponent
    {
        public Vector3 Position;
        public Vector3 Rotation;
        public Vector3 Scale = new(1f, 1f, 1f);
    }

    [StructLayout(LayoutKind.Sequential)]
    class ScriptComponent
    {
        public IntPtr ScriptCreator;
    }

    [StructLayout(LayoutKind.Sequential)]
    class GameEntityDescriptor
    {
        public TransformComponent Transform = new();
        public ScriptComponent Script = new();
    }
}

namespace MooncastleEditor.DllWrappers
{
    static class EngineAPI
    {
        private const string _engineDll = "EngineDLL.dll";

        [DllImport(_engineDll, CharSet = CharSet.Ansi)]
        public static extern int LoadGameCodeDll(string dllPath);
        [DllImport(_engineDll)]
        public static extern int UnloadGameCodeDll();
        [DllImport(_engineDll)]
        public static extern IntPtr GetScriptCreator(string name);
        [DllImport(_engineDll)]
        [return: MarshalAs(UnmanagedType.SafeArray)]
        public static extern string[] GetScriptNames();
        [DllImport(_engineDll)]
        public static extern int CreateRenderSurface(IntPtr host, int width, int height);
        [DllImport(_engineDll)]
        public static extern void RemoveRenderSurface(int surfaceId);
        [DllImport(_engineDll)]
        public static extern void ResizeRenderSurface(int surfaceId);
        [DllImport(_engineDll)]
        public static extern IntPtr GetWindowHandle(int surfaceId);

        internal static class EntityAPI
        {
            [DllImport(_engineDll)]
            private static extern int CreateGameEntity(GameEntityDescriptor entityDescriptor);
            public static int CreateGameEntity(GameEntity entity)
            {
                GameEntityDescriptor descriptor = new();

                //Transform
                {
                    var c = entity.GetComponent<Transform>();
                    descriptor.Transform.Position = c.Position;
                    descriptor.Transform.Rotation = c.Rotation;
                    descriptor.Transform.Scale = c.Scale;
                }
                //Script
                {
                    /*
                     * NOTE: Here we also check if current project is not null, so we can tell whether the game code DLL
                     * has been loaded or not. This way, creation of entities with a script 
                     * component is deferred until the DLL has been loaded.
                     */
                    var c = entity.GetComponent<Script>();

                    if (c != null && Project.Current != null)
                    {
                        if (Project.Current.AvailableScripts.Contains(c.Name))
                        {
                            descriptor.Script.ScriptCreator = GetScriptCreator(c.Name);
                        }
                        else
                        {
                            Logger.Log(MessageType.Error, $"Unable to find script with name {c.Name}. Game entity will be created without script component!");
                        }
                    }
                }

                return CreateGameEntity(descriptor);
            }

            [DllImport(_engineDll)]
            private static extern void RemoveGameEntity(int entityId);
            public static void RemoveGameEntity(GameEntity entity)
            {
                RemoveGameEntity(entity.EntityId);
            }
        }
    }
}
