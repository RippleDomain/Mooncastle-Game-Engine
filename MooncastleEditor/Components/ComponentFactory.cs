using MooncastleEditor.Content;
using System.Diagnostics;

namespace MooncastleEditor.Components;

enum ComponentType
{
    Transform,
    Script,
    Geometry
}

static class ComponentFactory
{
    private static readonly Func<GameEntity, object, Component>[] _function =
    [
        (entity, data) => new Transform(entity),
        (entity, data) => new Script(entity) { Name = (string)data },
        (entity, data) => new Geometry(entity, (AssetInfo)data)
    ];

    public static Func<GameEntity, object, Component> GetCreationFunction(ComponentType componentType)
    {
        Debug.Assert((int)componentType < _function.Length);

        return _function[(int)componentType];
    }

    public static ComponentType ToEnumType(this Component component)
    {
        return component switch
        {
            Transform _ => ComponentType.Transform,
            Script _ => ComponentType.Script,
            Geometry _ => ComponentType.Geometry,
            _ => throw new ArgumentException("Unknown component type"),
        };
    }
}
