using MooncastleEditor.Components;
using MooncastleEditor.EngineAPIStructs;
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
        public Vector3 Scale = new Vector3(1f, 1f, 1f);
    }

    [StructLayout(LayoutKind.Sequential)]
    class GameEntityDescriptor
    {
        public TransformComponent Transform = new TransformComponent();
    }
}

namespace MooncastleEditor.DllWrappers
{
    static class EngineAPI
    {
        private const string _dllName = "EngineDLL.dll";

        [DllImport(_dllName)]
        private static extern int CreateGameEntity(GameEntityDescriptor entityDescriptor);
        public static int CreateGameEntity(GameEntity entity)
        {
            GameEntityDescriptor descriptor = new GameEntityDescriptor();

            //Transform
            {
                var c = entity.GetComponent<Transform>();
                descriptor.Transform.Position = c.Position;
                descriptor.Transform.Rotation = c.Rotation;
                descriptor.Transform.Scale = c.Scale;
            }

            return CreateGameEntity(descriptor);
        }

        [DllImport(_dllName)]
        private static extern void RemoveGameEntity(int entityId);
        public static void RemoveGameEntity(GameEntity entity)
        {
            RemoveGameEntity(entity.EntityId);
        }
    }
}
